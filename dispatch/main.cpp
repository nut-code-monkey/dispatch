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
    dispatch::Queue queue = dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::HIGH);

    dispatch::Queue main_queue = dispatch::get_main_queue();
    for (int i = 0; i < 20; ++i)
    {
        dispatch::async(queue, [=](){
            
            std::string first_string = std::to_string(i);

            dispatch::async(main_queue, [=]{
                
                std::string second_string = std::to_string(i+1);
                
                std::cout << first_string << " -> " << second_string << std::endl;
            });
        });
    }

    dispatch::async(dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::BACKGROUND), [=]{
        dispatch::async(main_queue, [](){
            std::cout << "exit" << std::endl;
            dispatch::exit();
        });
    });

    dispatch::main_loop([]{
        std::cout << ".";
    });

    return 0;
}

