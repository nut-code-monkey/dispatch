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

    typedef std::function<void ()> Functor;
    
    class QueueImpl;
    typedef std::shared_ptr<QueueImpl> Queue;
    
    Queue get_main_queue();
    Queue get_queue_with_priority(QUEUE_PRIORITY priority);

    void process_main_loop();
    void main_loop(Functor function);
    void async(Queue queue, Functor function);
    void exit();
}

#endif
