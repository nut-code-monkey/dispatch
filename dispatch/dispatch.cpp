//
//  dispatch.cpp
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#include "dispatch.h"

#include <algorithm>
#include <deque>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace dispatch
{
#pragma mark - PriorityzedTask
    
    struct PriorityzedTask
    {
        PriorityzedTask(QUEUE_PRIORITY priority, Function task) : priority(priority), execute(task), timestamp(std::clock()){}

        const QUEUE_PRIORITY priority;
        const Function execute;
        const std::clock_t timestamp;
    private:
        PriorityzedTask(): priority(DEFAULT), execute(nullptr), timestamp(0){}
    };
    
#pragma mark - QueueImpl
    
    typedef std::shared_ptr<PriorityzedTask> QueueTask;
    
    struct QueueImpl
    {
        const QUEUE_PRIORITY priority;
        
        typedef std::function<void (Function)> AsyncRunner;
        const AsyncRunner async;

        QueueImpl(QUEUE_PRIORITY priority, AsyncRunner async) : priority(priority), async(async) {};
    };
    
#pragma mark - ThreadPool
    
    struct ThreadPool
    {
        ThreadPool(unsigned number_of_threads);
        
        typedef std::shared_ptr<ThreadPool> SharedPool;
        static SharedPool& instance();
        
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
    
        friend class QueueImpl;
    };
    
#pragma mark - QueueTask comparator
    
    bool operator<(const QueueTask& firs_task, const QueueTask& second_task)
    {
        if ( firs_task->priority == second_task->priority )
            return firs_task->timestamp > second_task->timestamp;
        else
            return firs_task->priority < second_task->priority;
    }
    
#pragma mark - ThreadPool
    
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

        for(size_t i = 0;i<workers.size();++i)
            workers[i].join();
    }
    
    ThreadPool::SharedPool& ThreadPool::instance()
    {
        static std::once_flag flag;
        static ThreadPool::SharedPool instance;
        std::call_once(flag, []
        {
            unsigned default_number_of_threads = 3;
            unsigned number_of_threads = std::thread::hardware_concurrency();
            instance = std::make_shared<ThreadPool>(number_of_threads?: default_number_of_threads);
        });
        return instance;
    }
    
#pragma mark - dispatch
    
    Queue get_main_queue()
    {
        return std::make_shared<QueueImpl>(HIGH, [](Function task)
        {
            std::unique_lock<std::mutex> lock(ThreadPool::instance()->main_queue_mutex);
            ThreadPool::instance()->main_queue.push(std::make_shared<PriorityzedTask>(HIGH, task));
            if (ThreadPool::instance()->main_loop_need_update != nullptr)
                ThreadPool::instance()->main_loop_need_update();
        });
    }
    
    Queue get_queue_with_priority(QUEUE_PRIORITY priority)
    {
        return std::make_shared<QueueImpl>(priority, [=](Function task)
        {
            {
                std::unique_lock<std::mutex> lock(ThreadPool::instance()->mutex);
                ThreadPool::instance()->priority_queue.push(std::make_shared<PriorityzedTask>(priority, task));
            }
            ThreadPool::instance()->condition.notify_one();
        });
    }
    
#pragma mark -
    
    void process_main_loop()
    {
        ThreadPool::SharedPool pool = ThreadPool::instance();
        std::unique_lock<std::mutex> lock(pool->main_queue_mutex);
        while (!pool->main_queue.empty())
        {
            QueueTask task = pool->main_queue.front();
            pool->main_queue.pop();
            task->execute();
        }
    }

    void main_loop(Function function)
    {
        Queue main_queue = get_main_queue();
        while (!ThreadPool::instance()->stop)
        {
            async(main_queue, function);
            process_main_loop();
        }
    }

    void async(Queue queue, Function function)
    {
        queue->async(function);
    }
    
    void exit()
    {
        ThreadPool::instance()->stop = true;
    }
    
    void set_main_loop_process_callback(Function update_callback)
    {
        ThreadPool::instance()->main_loop_need_update = update_callback;
    }
}
