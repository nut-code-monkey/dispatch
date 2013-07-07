//
//  dispatch.cpp
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#include "dispatch.h"

#include <algorithm>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace dispatch
{
    struct PriorityzedTask
    {
        PriorityzedTask(QUEUE_PRIORITY::Priority priority, Function task): priority(priority), execute(task), timestamp(std::clock()){}

        const QUEUE_PRIORITY::Priority priority;
        const Function execute;
        const std::clock_t timestamp;
    };

    typedef std::shared_ptr<PriorityzedTask> QueueTask;
    
    struct ThreadPool
    {
        ThreadPool(unsigned number_of_threads);

        static std::shared_ptr<ThreadPool>& shared_pool();
        
        virtual ~ThreadPool();
        
        bool stop;
        
        std::priority_queue<QueueTask> priority_queue;
        std::mutex mutex;
        std::condition_variable condition;
        
        std::queue<QueueTask> main_queue;
        std::mutex main_queue_mutex;
        
        Function main_loop_need_update;
    private:
        std::vector<std::thread> workers;
        void add_worker();
    };

    bool operator<(const QueueTask& firs_task, const QueueTask& second_task)
    {
        if ( firs_task->priority == second_task->priority )
            return firs_task->timestamp > second_task->timestamp;
        else
            return firs_task->priority < second_task->priority;
    }
    
    void ThreadPool::add_worker()
    {
        workers.push_back(std::thread([&]
        {
            QueueTask task;
            while(true)
            {
                {
                    std::unique_lock<std::mutex> lock(mutex);

                    while(!stop && priority_queue.empty())
                        condition.wait(lock);

                    if(stop) // exit if the pool is stopped
                        return;

                    task = priority_queue.top();
                    priority_queue.pop();
                }
                task->execute();
            }
        }));
    }
    
    ThreadPool::ThreadPool(unsigned number_of_threads) : stop(false), main_loop_need_update(nullptr)
    {
        for (size_t i = 0; i < number_of_threads; ++i)
            add_worker();
    }
    
    ThreadPool::~ThreadPool()
    {
        stop = true;
        condition.notify_all();

        for(size_t i = 0; i < workers.size(); ++i)
            workers[i].join();
    }
    
    std::shared_ptr<ThreadPool>& ThreadPool::shared_pool()
    {
        static std::once_flag flag;
        static std::shared_ptr<ThreadPool> shared_pool;
        std::call_once(flag, []
        {
            unsigned default_number_of_threads = 5;
            unsigned number_of_threads = std::thread::hardware_concurrency();
            shared_pool = std::make_shared<ThreadPool>(number_of_threads?: default_number_of_threads);
        });
        return shared_pool;
    }
    
    std::shared_ptr<Queue> get_main_queue()
    {
        return std::make_shared<Queue>(QUEUE_PRIORITY::DEFAULT, [](Function task)
        {
            auto thread_pool = ThreadPool::shared_pool();
            std::unique_lock<std::mutex> lock(thread_pool->main_queue_mutex);
            thread_pool->main_queue.push(std::make_shared<PriorityzedTask>(QUEUE_PRIORITY::HIGH, task));
            if (thread_pool->main_loop_need_update != nullptr)
                thread_pool->main_loop_need_update();
        });
    }
    
    std::shared_ptr<Queue> get_queue_with_priority(QUEUE_PRIORITY::Priority priority)
    {
        return std::make_shared<Queue>(priority, [=](Function task)
        {
            auto thread_pool = ThreadPool::shared_pool();
            {
                std::unique_lock<std::mutex> lock(thread_pool->mutex);
                thread_pool->priority_queue.push(std::make_shared<PriorityzedTask>(priority, task));
            }
            thread_pool->condition.notify_one();
        });
    }
    
    void process_main_loop()
    {
        auto thread_pool = ThreadPool::shared_pool();
        std::unique_lock<std::mutex> lock(thread_pool->main_queue_mutex);
        while (!thread_pool->main_queue.empty())
        {
            QueueTask task = thread_pool->main_queue.front();
            thread_pool->main_queue.pop();
            task->execute();
        }
    }

    void main_loop(Function function)
    {
        auto main_queue = get_main_queue();
        while (!ThreadPool::shared_pool()->stop)
        {
            main_queue->async(function);
            process_main_loop();
        }
    }
    
    void exit()
    {
        ThreadPool::shared_pool()->stop = true;
    }
    
    void set_main_loop_process_callback(Function update_callback)
    {
        ThreadPool::shared_pool()->main_loop_need_update = update_callback;
    }
}
