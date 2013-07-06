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

namespace dispatch
{
    typedef enum
    {
        HIGH = 2,
        DEFAULT = 0,
        LOW = (-2),
        BACKGROUND = INT16_MIN
    } QUEUE_PRIORITY;

    typedef std::function<void ()> Function;
    
    struct Queue
    {
        Queue(QUEUE_PRIORITY priority, std::function<void (Function)> async) : priority(priority), async(async) {};
        
        const QUEUE_PRIORITY priority;
        const std::function<void (Function)> async;
    };
    
    std::shared_ptr<Queue> get_main_queue();
    std::shared_ptr<Queue> get_queue_with_priority(QUEUE_PRIORITY priority);

    void exit();
    void main_loop(Function function);
    void process_main_loop();
    void set_main_loop_process_callback(Function function);
}

#endif
