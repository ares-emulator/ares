//
//  main.m
//  objctest
//
//  Created by Ronny Chan on 2024-02-13.
//

#import "librashader.h"

#import <Cocoa/Cocoa.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
    }
    
//    libra_instance_t librashader = librashader_load_instance();
    
    printf("%d", LIBRASHADER_CURRENT_VERSION);
    return NSApplicationMain(argc, argv);
}
