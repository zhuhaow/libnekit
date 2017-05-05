// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#import <UIKit/UIApplication.h> // UIApplicationDelegate
#import <UIKit/UIResponder.h> // UIResponder

@class UIWindow;

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window; // The app delegate
    // must implement the window property if it wants to
    // use a main storyboard file

@end
