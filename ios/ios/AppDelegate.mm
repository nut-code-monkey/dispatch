//
//  AppDelegate.m
//  ios
//
//  Created by Max on 05.07.13.
//  Copyright (c) 2013 Max Lunin. All rights reserved.
//

#import "AppDelegate.h"
#import "dispatch.h"

@interface AppDelegate ()

@property (assign, nonatomic) CFRunLoopObserverRef runLoopObserver;

@end

@implementation AppDelegate

@synthesize runLoopObserver;

static void onRunLoop(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void* info)
{
    dispatch::process_main_loop();
}

- (void)attachObservers
{
    if (runLoopObserver != NULL)
        return;

    CFRunLoopObserverContext context =
    {
        0,    // Version of this structure. Must be zero.
        (__bridge void*)self, // Info pointer: a reference to this UpdateTimer.
        NULL, // Retain callback for info pointer.
        NULL, // Release callback for info pointer.
        NULL  // Copy description.
    };

    runLoopObserver = CFRunLoopObserverCreate(NULL,
                                              kCFRunLoopBeforeWaiting | kCFRunLoopExit, // Observe when the run loop is waiting and just before it exits.
                                              true, // Repeats.
                                              0, // Priority index. Use zero because there's currently no reason to do otherwise.
                                              onRunLoop,
                                              &context); // Copied by CFRunLoopObserverCreate, no need to pass a heap pointer.

    CFRunLoopAddObserver(CFRunLoopGetCurrent(), runLoopObserver, kCFRunLoopCommonModes);
}

- (void)detachObservers
{
    if (runLoopObserver == NULL)
        return;

    CFRunLoopObserverInvalidate(runLoopObserver);
    CFRelease(runLoopObserver);
    runLoopObserver = NULL;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [self attachObservers];
    
    // Override point for customization after application launch.
    return YES;
}
							
- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    [self detachObservers];
    
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
