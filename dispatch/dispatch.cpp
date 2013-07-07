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
#include <map>
#include <mutex>
#include <queue>
#include <thread>

namespace dispatch
{
    class ThreadPool;
    class QueueImpl
    {
    public:
        QueueImpl();
        void addTask(Function task);
    private:
        const std::shared_ptr<QueueImpl> queue_impl;
        
        std::mutex mutex;
        std::queue<Function> queue;
        std::condition_variable condition;
        std::thread thread;
        
        friend class ThreadPool;
    };
    
    struct ThreadPool
    {
        ThreadPool();
        
        static std::shared_ptr<ThreadPool>& shared_pool();
        
        virtual ~ThreadPool();
        
        bool stop;
        
        std::shared_ptr<QueueImpl> queue_with_priority(Queue::Priority);

        std::mutex mutex;
        
        std::mutex main_thread_mutex;
        std::queue<Function> main_queue;
        
        Function main_loop_need_update;
    private:
        friend class QueueImpl;
        std::map<Queue::Priority, std::shared_ptr<QueueImpl>> queues;

        std::shared_ptr<QueueImpl> add_queue_with_priority(Queue::Priority);
        void remove_queue_with_priority(Queue::Priority);
    };

    QueueImpl::QueueImpl()
    {
        thread = std::thread([&]
        {
            auto pool = ThreadPool::shared_pool();

            Function task;
            while(true)
            {
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    
                    while(!pool->stop && queue.empty())
                        condition.wait(lock);
                    
                    if(pool->stop) // exit if the pool is stopped
                        return;
                    
                    task = queue.front();
                    queue.pop();
                }
                task();
            }
        });
    }
    
    void QueueImpl::addTask(Function task)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            queue.push(task);
        }
        condition.notify_one();
    }
    
    void Queue::async(Function task) const
    {
        ThreadPool::shared_pool()->queue_with_priority(priority)->addTask(task);
    };
    
    std::shared_ptr<Queue> Queue::queue_with_priority(Priority priority)
    {
        return std::make_shared<Queue>(priority);
    }
    
    std::shared_ptr<QueueImpl> ThreadPool::add_queue_with_priority(Queue::Priority priority)
    {
        std::unique_lock<std::mutex> lock(mutex);
        auto queue = queues[priority];
        if (queue == nullptr)
        {
            queue = std::make_shared<QueueImpl>();
            queues[priority] = queue;
        }
        return queue;
    }
    
    std::shared_ptr<QueueImpl> ThreadPool::queue_with_priority(Queue::Priority priority)
    {
        auto queue = queues[priority];
        if (queue == nullptr)
        {
            queue = add_queue_with_priority(priority);
        }
        return queue;
    }
    
    ThreadPool::ThreadPool() : stop(false), main_loop_need_update(nullptr)
    {
        add_queue_with_priority(QUEUE_PRIORITY::HIGH);
        add_queue_with_priority(QUEUE_PRIORITY::DEFAULT);
        add_queue_with_priority(QUEUE_PRIORITY::LOW);
        add_queue_with_priority(QUEUE_PRIORITY::BACKGROUND);
    }
    
    ThreadPool::~ThreadPool()
    {
        stop = true;
        for (auto iterator: queues)
        {
            iterator.second->condition.notify_all();
            iterator.second->thread.join();
        }
    }
    
    std::shared_ptr<ThreadPool>& ThreadPool::shared_pool()
    {
        static std::once_flag flag;
        static std::shared_ptr<ThreadPool> shared_pool;
        std::call_once(flag, []
        {
            shared_pool = std::make_shared<ThreadPool>();
        });
        return shared_pool;
    }
    
    struct MainQueue : Queue
    {
        virtual void async(Function task) const override;
        MainQueue(): Queue(0) {};
    };

    void MainQueue::async(Function task) const 
    {
        auto pool = ThreadPool::shared_pool();
        std::unique_lock<std::mutex> lock(pool->main_thread_mutex);
        pool->main_queue.push(task);
        if (pool->main_loop_need_update != nullptr)
            pool->main_loop_need_update();
    }
    
    std::shared_ptr<Queue> Queue::main_queue()
    {
        return std::static_pointer_cast<Queue>(std::make_shared<MainQueue>());
    }
    
    void process_main_loop()
    {
        auto pool = ThreadPool::shared_pool();
        std::unique_lock<std::mutex> lock(pool->main_thread_mutex);
        while (!pool->main_queue.empty())
        {
            Function task = pool->main_queue.front();
            pool->main_queue.pop();
            task();
        }
    }

    void main_loop(Function function)
    {
        auto main_queue = Queue::main_queue();
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
