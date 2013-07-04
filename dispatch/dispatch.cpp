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
    typedef std::function<void ()> Functor;
    class ThreadPool;
    
#pragma mark - Task
    class TaskPriority{
    public:
        friend class ThreadPool;
        friend bool operator<(const TaskPriority&, const TaskPriority&);

        TaskPriority(QUEUE_PRIORITY priority, Functor task) : priority(priority), task(task) {}
        
    private:
        QUEUE_PRIORITY priority;
        Functor task;
    };

    bool operator<(const TaskPriority& firsTask, const TaskPriority& secondTask) {
        return firsTask.priority < secondTask.priority;
    }
    
#pragma mark - Queue

    class ThreadPool;

    typedef std::function<void (Functor)> AppendTask;
    
    class Queue {
    public:
        QUEUE_PRIORITY priority;
        AppendTask append_task;
        Queue(AppendTask append): append_task(append) {};
        Queue(QUEUE_PRIORITY priority, AppendTask append) : priority(priority), append_task(append) {};
        
        void add_task(Functor task);
    private:
        Queue(){};
    };
    
#pragma mark - ThreadPool
    
    class ThreadPool {
    public:
        ThreadPool(size_t);
        static ThreadPool* instance();
        ~ThreadPool();
        
        bool stop;
        
        std::priority_queue<std::shared_ptr<TaskPriority>> tasks;
        
        std::mutex queue_mutex;
        std::condition_variable condition;
        
        std::vector<std::shared_ptr<TaskPriority>> main_queue;
        std::mutex main_mutex;
        std::condition_variable main_condition;
        
    private:
        friend class Queue;
        
        std::vector<std::thread> workers;
        std::thread main_thread;
        
        void add_worker();
    };
    
#pragma mark - Queue
    
    void Queue::add_task(Functor task){
        append_task(task);
    }
        
#pragma mark - ThreadPool
    
    void ThreadPool::add_worker(){    
        workers.push_back(std::thread([&]{
            std::shared_ptr<TaskPriority> task;
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
    
    ThreadPool::ThreadPool(size_t threads) : stop(false){
        for (int i = 0; i < threads; ++i)
            add_worker();
        
        
        main_thread = std::thread([&]{
            std::shared_ptr<TaskPriority> task;
            while(true)
            {
                {
                    std::unique_lock<std::mutex> lock(this->main_mutex);
                    
                    while(!stop && main_queue.empty())
                        main_condition.wait(lock);
                    
                    if(stop) // exit if the pool is stopped
                        return;
                    
                    task = main_queue.back();
                    main_queue.pop_back();
                }
                task->task();
            }
        });
    }
    
    ThreadPool::~ThreadPool(){
        stop = true;
        condition.notify_all();
        
        // the destructor joins all threads
        for(size_t i = 0;i<workers.size();++i)
            workers[i].join();
        
        main_thread.join();
    }
    
    ThreadPool* ThreadPool::instance(){
        static std::once_flag flag;
        static ThreadPool* instance = nullptr;
        std::call_once(flag, [](){
                           int default_not_computable_value = 5;
                           int number_of_threads = std::thread::hardware_concurrency();
                           instance = new ThreadPool(number_of_threads?: default_not_computable_value);
                       });
        return instance;
    }
    
#pragma mark -
    
    std::shared_ptr<Queue> get_main_queue(){
        return std::make_shared<Queue>([](Functor task) {
            ThreadPool* pool = ThreadPool::instance();
            {
                std::unique_lock<std::mutex> lock(pool->main_mutex);
                pool->main_queue.push_back(std::make_shared<TaskPriority>(QUEUE_PRIORITY::DEFAULT, task));
            }
            
            pool->main_condition.notify_one();
        });
    }
    
    void main_loop(Functor function){
        while (!ThreadPool::instance()->stop){
            function();
        }
    }

    void async(std::shared_ptr<Queue> queue, Functor function){
        queue->add_task(function);
    }
    
    void exit(){
        ThreadPool::instance()->stop = true;
    }
    
    std::shared_ptr<Queue> get_queue_with_priority(QUEUE_PRIORITY priority){
        return std::make_shared<Queue>(priority, [=](Functor task) {
            ThreadPool* pool = ThreadPool::instance();
            {
                std::unique_lock<std::mutex> lock(pool->queue_mutex);
                pool->tasks.push(std::make_shared<TaskPriority>(priority, task));
            }
            
            pool->condition.notify_one();
        });
    }
}
