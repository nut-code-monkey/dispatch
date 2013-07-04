//
//  main.cpp
//  dispatch
//
//  Created by Max on 30.06.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#include <iostream>
#include <string>
#include <sstream>

#include "dispatch.h"

int main(int argc, const char * argv[])
{
    std::string hello("hello");
    
    dispatch::async(dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::DEFAULT), [=]{
        
        std::stringstream ss;
        ss << hello << " " << "world";
        std::string hello_world = ss.str();
        
        dispatch::async(dispatch::get_queue_with_priority(dispatch::QUEUE_PRIORITY::DEFAULT), [=]{
            std::cout << hello_world << std::endl;
            
            dispatch::async(dispatch::get_main_queue(), []{
                dispatch::exit();
            });
            
        });
    });

    dispatch::main_loop([]{
        std::cout << ".";
    });
    
    return 0;
}

