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
#pragma mark - TaskPriority
    
    struct TaskPriority
    {
        TaskPriority(QUEUE_PRIORITY priority, Functor task) : priority(priority), task(task), timestamp(std::clock()){}

        QUEUE_PRIORITY priority;
        Functor task;
        std::clock_t timestamp;
    };

#pragma mark - Queue
    
    typedef std::function<void (Functor)> AppendTask;
    
    struct QueueImpl
    {
        QUEUE_PRIORITY priority;
        AppendTask append_task;

        QueueImpl(AppendTask append): append_task(append) {};
        QueueImpl(QUEUE_PRIORITY priority, AppendTask append) : priority(priority), append_task(append) {};
        
        typedef std::shared_ptr<TaskPriority> Task;
        
    private:
        QueueImpl(){};
    };
    
#pragma mark - ThreadPool
    
    class ThreadPool
    {
    public:
        ThreadPool(size_t);
        static ThreadPool* instance();
        ~ThreadPool();
        
        bool stop;
        
        std::priority_queue<QueueImpl::Task> tasks;
        
        std::mutex queue_mutex;
        std::condition_variable condition;
        
        std::queue<QueueImpl::Task> main_queue;
        std::mutex main_mutex;
        std::condition_variable main_condition;
        
        Functor main_loop_need_update;
    private:
        friend class QueueImpl;

        std::vector<std::thread> workers;
        void add_worker();
    };
    
#pragma mark - Task comparator
    
    bool operator<(const QueueImpl::Task& firsTask, const QueueImpl::Task& secondTask)
    {
        if ( firsTask->priority == secondTask->priority )
            return firsTask->timestamp > secondTask->timestamp;
        else
            return firsTask->priority < secondTask->priority;
    }
    
#pragma mark - ThreadPool
    
    void ThreadPool::add_worker()
    {
        workers.push_back(std::thread([&]
        {
            QueueImpl::Task task;
            while(true)
            {
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);

                    while(!stop && tasks.empty())
                        condition.wait(lock);

                    if(stop) // exit if the pool is stopped
                        return;

                    task = tasks.top();
                    tasks.pop();
                }
                task->task();
            }
        }));
    }
    
    ThreadPool::ThreadPool(size_t threads) : stop(false)
    {
        for (int i = 0; i < threads; ++i)
            add_worker();
    }
    
    ThreadPool::~ThreadPool()
    {
        stop = true;
        condition.notify_all();

        for(size_t i = 0;i<workers.size();++i)
            workers[i].join();
    }
    
    ThreadPool* ThreadPool::instance()
    {
        static std::once_flag flag;
        static ThreadPool* instance = nullptr;
        std::call_once(flag, []()
                       {
                           int default_not_computable_value = 3;
                           int number_of_threads = std::thread::hardware_concurrency();
                           instance = new ThreadPool(number_of_threads?: default_not_computable_value);
                       });
        return instance;
    }
    
#pragma mark -
    
    Queue get_main_queue()
    {
        return std::make_shared<QueueImpl>([](Functor task)
        {
            std::unique_lock<std::mutex> lock(ThreadPool::instance()->main_mutex);
            ThreadPool::instance()->main_queue.push(std::make_shared<TaskPriority>(QUEUE_PRIORITY::HIGH, task));
            if (ThreadPool::instance()->main_loop_need_update != nullptr)
                ThreadPool::instance()->main_loop_need_update();
        });
    }
    
    Queue get_queue_with_priority(QUEUE_PRIORITY priority)
    {
        return std::make_shared<QueueImpl>(priority, [=](Functor task)
                                           {
                                               {
                                                   std::unique_lock<std::mutex> lock(ThreadPool::instance()->queue_mutex);
                                                   ThreadPool::instance()->tasks.push(std::make_shared<TaskPriority>(priority, task));
                                               }
                                               ThreadPool::instance()->condition.notify_one();
                                           });
    }
    
    void process_main_loop()
    {
        ThreadPool* pool = ThreadPool::instance();
        std::unique_lock<std::mutex> lock(pool->main_mutex);
        while (!pool->main_queue.empty())
        {
            QueueImpl::Task task = pool->main_queue.front();
            pool->main_queue.pop();
            task->task();
        }
    }

    void main_loop(Functor function)
    {
        Queue main_queue = get_main_queue();
        while (!ThreadPool::instance()->stop)
        {
            async(main_queue, function);
            process_main_loop();
        }
    }

    void async(Queue queue, Functor function)
    {
        queue->append_task(function);
    }
    
    void exit()
    {
        ThreadPool::instance()->stop = true;
    }
    
    void set_main_loop_process_callback(Functor update_callback)
    {
        ThreadPool::instance()->main_loop_need_update = update_callback;
    }
}
