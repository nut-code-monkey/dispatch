//
//  ViewController.m
//  ios
//
//  Created by Max on 05.07.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#import "ViewController.h"

#import "dispatch.h"

#import <string>
#import <iostream>
#import <thread>

@implementation ViewController

- (IBAction)onRunButtonClick:(id)sender
{
    std::cout << "number of threads : " << std::thread::hardware_concurrency() << std::endl;
    
    for (int i = 0; i < 20; ++i)
    {
        dispatch::queue(dispatch::QUEUE_PRIORITY::DEFAULT).async([=]
        {
            NSAssert(![NSThread isMainThread], nil);
            
            std::string first_string = std::to_string(i);
            
            dispatch::queue::main_queue()->async([=]
            {
                NSAssert([NSThread isMainThread], nil);
                
                std::string second_string = std::to_string(i+1);
                
                std::cout << first_string << " -> " << second_string << std::endl;
                
                [self.tableView reloadData];
            });
        });
    }
}

@end
