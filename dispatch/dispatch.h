//
//  dispatch.h
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#ifndef __dispatch__
#define __dispatch__

#include <memory>
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

    class Queue;
    typedef std::function<void ()> Functor;
    
    std::shared_ptr<Queue> get_main_queue();
    std::shared_ptr<Queue> get_queue_with_priority(QUEUE_PRIORITY priority);
    
    void main_loop(Functor function);
    void async(std::shared_ptr<Queue> queue, Functor function);
    void exit();
}

#endif
