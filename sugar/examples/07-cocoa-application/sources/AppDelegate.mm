// Copyright (c) 2013, Ruslan Baratov
// All rights reserved.

#import "AppDelegate.h"

#include <boost/concept_check.hpp> // boost::ignore_unused_variable_warning

@implementation AppDelegate

@synthesize window = _windows;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
  boost::ignore_unused_variable_warning(aNotification);
  // Insert code here to initialize your application
}

@end
