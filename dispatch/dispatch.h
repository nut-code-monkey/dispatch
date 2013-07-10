//
//  dispatch.h
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#ifndef __dispatch__
#define __dispatch__

#include <functional>
#include <memory>
#include <queue>

namespace dispatch
{
    typedef std::function<void ()> function;

    struct queue
    {
        typedef long priority;
        const priority queue_priority;

        static std::shared_ptr<queue> main_queue();
        
        virtual void async(function) const;

        queue(queue::priority priority) : queue_priority(priority) {};
    };
    
    namespace QUEUE_PRIORITY
    {
        queue::priority const HIGH = 2;
        queue::priority const DEFAULT = 0;
        queue::priority const LOW = (-2);
        queue::priority const BACKGROUND = (-255);
    };
    
    void exit();
    void main_loop(dispatch::function main_loop_function);
    
    void process_main_loop();
    void set_main_loop_process_callback(dispatch::function callback);
}

#endif
