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
#include <thread>
#include <assert.h>

int main(int argc, const char * argv[])
{
    auto main_thread_id = std::this_thread::get_id();
    
    for (int i = 0; i < 10; ++i)
    {
        dispatch::async(dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::HIGH), [=](){
            
            assert(std::this_thread::get_id() != main_thread_id);

            std::string first_string = std::to_string(i);
            
            dispatch::async(dispatch::get_main_queue(), [=]{

                assert(std::this_thread::get_id() == main_thread_id);
                
                std::string second_string = std::to_string(i+1);
                
                std::cout << first_string << " -> " << second_string << std::endl;
            });
        });
    }

    dispatch::async(dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::BACKGROUND), [=]{
        dispatch::async(dispatch::get_main_queue(), [](){
            std::cout << "exit" << std::endl;
            dispatch::exit();
        });
    });

    dispatch::main_loop([]{
        std::cout << ".";
    });

    return 0;
}

