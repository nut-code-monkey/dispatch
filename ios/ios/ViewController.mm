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

@interface ViewController ()

@end

@implementation ViewController

- (IBAction)onRunButtonClick:(id)sender
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
    
}
@end
