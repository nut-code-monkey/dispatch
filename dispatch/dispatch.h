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
    typedef std::function<void ()> Function;
    
    struct Queue
    {
        typedef long Priority;
        
        static std::shared_ptr<Queue> main_queue();

        virtual void async(Function) const;

        Queue(Priority priority) : priority(priority) {};
        const Priority priority;
    };
    
    namespace QUEUE_PRIORITY
    {
        Queue::Priority const HIGH = 2;
        Queue::Priority const DEFAULT = 0;
        Queue::Priority const LOW = (-2);
        Queue::Priority const BACKGROUND = (-255);
    };
    
    void exit();
    void main_loop(Function function);
    
    void process_main_loop();
    void set_main_loop_process_callback(Function function);
}

#endif
