//
//  main.cpp
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#include "dispatch.h"

#include <iostream>
#include <string>
#include <sstream>

int main(int argc, const char * argv[])
{
    std::string hello("hello");
    
    dispatch::Queue queue = dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::HIGH);

    dispatch::Queue main_queue = dispatch::get_main_queue();
    for (int i = 0; i < 20; ++i)
    {
        dispatch::async(queue, [=](){
            int res = i + 1;
            dispatch::async(main_queue, [=]{
                std::cout << res << std::endl;
            });
        });
    }

    dispatch::async(dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::BACKGROUND), []{
        
        dispatch::exit();
    });

    dispatch::main_loop([]{
        std::cout << ".";
    });
    
    return 0;
}

