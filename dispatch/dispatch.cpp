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
#include <math.h>

namespace dispatch
{
    struct queue_impl
    {
        const queue::priority priority;
        std::queue<function> tasks;
        bool is_running;
        queue_impl(queue::priority priority): priority(priority){};
    };
    
    struct thread_pool
    {
        thread_pool();
        static std::shared_ptr<thread_pool>& shared_pool();
        virtual ~thread_pool();
        
        bool stop;
        
        typedef std::shared_ptr<queue_impl> queue_ptr;
        
        void push_task_with_priority(const dispatch::function&, queue::priority);
        bool get_free_queue(queue_ptr*) const;
        void start_task_in_queue(const queue_ptr&);
        void stop_task_in_queue(const queue_ptr&);
        
        std::mutex mutex;
        std::map<queue::priority, queue_ptr> queues;
        
        std::mutex main_thread_mutex;
        std::queue<dispatch::function> main_queue;
        
        std::condition_variable condition;
        std::vector<std::thread> threads;
        
        dispatch::function main_loop_need_update;
        void add_worker();
    };
    
    bool thread_pool::get_free_queue(queue_ptr* out_queue) const
    {
        auto finded = std::find_if(queues.rbegin(), queues.rend(), [](const std::pair<queue::priority, queue_ptr>& iterator)
                                     {
                                         return !iterator.second->is_running;
                                     });
        
        bool is_free_queue_exist = (finded != queues.rend());
        if (is_free_queue_exist)
            *out_queue = finded->second;
        
        return  is_free_queue_exist;
    }
    
    void thread_pool::start_task_in_queue(const queue_ptr& queue)
    {
        queue->is_running = true;
    }

    void thread_pool::push_task_with_priority(const dispatch::function& task, queue::priority priority)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);

            auto queue = queues[priority];
            if (!queue)
            {
                queue = std::make_shared<queue_impl>(priority);
                queues[priority] = queue;
            }
            
            queue->tasks.push(task);
            
            unsigned max_number_of_threads = std::max<unsigned>(std::thread::hardware_concurrency(), 2);
            unsigned number_of_threads_required = round(log(queues.size()) + 1);
            while (threads.size() < std::min<unsigned>(max_number_of_threads, number_of_threads_required))
            {
                add_worker();
            }
        }
        condition.notify_one();
    }
    
    void thread_pool::stop_task_in_queue(const queue_ptr& queue)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);

            queue->is_running = false;
            if ( queue->tasks.size() ==0 )
            {
                queues.erase(queues.find(queue->priority));
            }
        }
        condition.notify_one();
    }
    
    void thread_pool::add_worker()
    {
        threads.push_back(std::thread([=]
                          {
                              dispatch::function task;
                              thread_pool::queue_ptr queue;
                              while(true)
                              {
                                  {
                                      std::unique_lock<std::mutex> lock(mutex);
                                      
                                      while(!stop && !get_free_queue(&queue))
                                          condition.wait(lock);
                                   
                                      if(stop)
                                          return;

                                      task = queue->tasks.front();
                                      queue->tasks.pop();

                                      start_task_in_queue(queue);
                                  }
                                  task();
                                  stop_task_in_queue(queue);
                              }
                          }));
    }
    
    thread_pool::thread_pool(){}
    
    void queue::async(dispatch::function task) const
    {
        thread_pool::shared_pool()->push_task_with_priority(task, this->queue_priority);
    };

    thread_pool::~thread_pool()
    {
        stop = true;
        condition.notify_all();
        for (auto& thread: threads)
        {
            thread.join();
        }
    }
    
    std::shared_ptr<thread_pool>& thread_pool::shared_pool()
    {
        static std::once_flag flag;
        static std::shared_ptr<thread_pool> shared_pool;
        std::call_once(flag, []
        {
            shared_pool = std::make_shared<thread_pool>();
        });
        return shared_pool;
    }
    
    struct main_queue : queue
    {
        virtual void async(dispatch::function task) const override;
        main_queue(): queue(0) {};
    };

    void main_queue::async(dispatch::function task) const 
    {
        auto pool = thread_pool::shared_pool();
        std::unique_lock<std::mutex> lock(pool->main_thread_mutex);
        pool->main_queue.push(task);
        if (pool->main_loop_need_update != nullptr)
            pool->main_loop_need_update();
    }
    
    std::shared_ptr<queue> queue::main_queue()
    {
        return std::static_pointer_cast<dispatch::queue>(std::make_shared<dispatch::main_queue>());
    }
    
    void process_main_loop()
    {
        auto pool = thread_pool::shared_pool();
        std::unique_lock<std::mutex> lock(pool->main_thread_mutex);
        while (!pool->main_queue.empty())
        {
            auto task = pool->main_queue.front();
            pool->main_queue.pop();
            task();
        }
    }

    void main_loop(dispatch::function main_loop_function)
    {
        auto main_queue = queue::main_queue();
        while (!thread_pool::shared_pool()->stop)
        {
            main_queue->async(main_loop_function);
            process_main_loop();
        }
    }
    
    void exit()
    {
        thread_pool::shared_pool()->stop = true;
    }
    
    void set_main_loop_process_callback(dispatch::function update_callback)
    {
        thread_pool::shared_pool()->main_loop_need_update = update_callback;
    }
}
