//
//  main.cpp
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#include "dispatch.h"

#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

int main(int argc, const char * argv[])
{
    auto main_thread_id = std::this_thread::get_id();

    for (size_t i = 0; i < 30; ++i)
    {
        dispatch::Queue(dispatch::QUEUE_PRIORITY::DEFAULT).async([=]
        {
            assert(std::this_thread::get_id() != main_thread_id);

            std::string first_string = std::to_string(i);
            
            dispatch::Queue::main_queue()->async([=]
            {
                assert(std::this_thread::get_id() == main_thread_id);
                
                std::string second_string = std::to_string(i+1);
                
                std::cout << first_string << " -> " << second_string << std::endl;
            });
        });
    }

    dispatch::Queue(dispatch::QUEUE_PRIORITY::DEFAULT).async([]
    {
        dispatch::Queue::main_queue()->async([]
        {
            std::cout << "exit" << std::endl;
            dispatch::exit();
        });
    });

    dispatch::main_loop([]{
        std::cout << ".";
    });
    
    return 0;
}

