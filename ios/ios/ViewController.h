//
//  ViewController.h
//  ios
//
//  Created by Max on 05.07.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface ViewController : UIViewController

-(IBAction)onRunButtonClick:(id)sender;

@property (strong, nonatomic) IBOutlet UITableView *tableView;

@end
