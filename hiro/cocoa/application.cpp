#if defined(Hiro_Application)

@implementation CocoaDelegate : NSObject

-(void) constructMenu {
    NSBundle* bundle = [NSBundle mainBundle];
    NSDictionary* dictionary = [bundle infoDictionary];
    NSString* applicationName = [dictionary objectForKey:@"CFBundleDisplayName"];

    menuBar = [[NSMenu alloc] init];

    NSMenuItem* item;
    string text;

    rootMenu = [[NSMenu alloc] init];
    item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [item setSubmenu:rootMenu];
    [menuBar addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"About %@…", applicationName] action:@selector(menuAbout) keyEquivalent:@""];
    [item setTarget:self];
    [rootMenu addItem:item];
    [rootMenu addItem:[NSMenuItem separatorItem]];

    item = [[NSMenuItem alloc] initWithTitle:@"Preferences…" action:@selector(menuPreferences) keyEquivalent:@""];
    [item setTarget:self];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    item.keyEquivalent = @",";
    [rootMenu addItem:item];

    [rootMenu addItem:[NSMenuItem separatorItem]];

    NSMenu* servicesMenu = [[NSMenu alloc] initWithTitle:@"Services"];
    item = [[NSMenuItem alloc] initWithTitle:@"Services" action:nil keyEquivalent:@""];
    [item setTarget:self];
    [item setSubmenu:servicesMenu];
    [rootMenu addItem:item];
    [rootMenu addItem:[NSMenuItem separatorItem]];
    [NSApp setServicesMenu:servicesMenu];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Hide %@", applicationName] action:@selector(hide:) keyEquivalent:@""];
    [item setTarget:NSApp];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    item.keyEquivalent = @"h";
    [rootMenu addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@""];
    [item setTarget:NSApp];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand | NSEventModifierFlagOption;
    item.keyEquivalent = @"h";
    [rootMenu addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [item setTarget:NSApp];
    [rootMenu addItem:item];

    [rootMenu addItem:[NSMenuItem separatorItem]];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Quit %@", applicationName] action:@selector(menuQuit) keyEquivalent:@""];
    [item setTarget:self];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    item.keyEquivalent = @"q";
    [rootMenu addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Edit"] action:nil keyEquivalent:@""];
    editMenu = [[NSMenu alloc] init];
    [item setSubmenu:editMenu];
    [menuBar addItem:item];
      
    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Cut"] action:@selector(cut:) keyEquivalent:@"x"];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [editMenu addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Copy"] action:@selector(copy:) keyEquivalent:@"c"];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [editMenu addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Paste"] action:@selector(paste:) keyEquivalent:@"v"];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [editMenu addItem:item];

    item = [[NSMenuItem alloc] initWithTitle:[NSString stringWithFormat:@"Select All"] action:@selector(selectAll:) keyEquivalent:@"a"];
    item.keyEquivalentModifierMask = NSEventModifierFlagCommand;
    [editMenu addItem:item];
    
    [NSApp setMainMenu:menuBar];
}

-(void) menuAbout {
  hiro::Application::Cocoa::doAbout();
}

-(void) menuPreferences {
  hiro::Application::Cocoa::doPreferences();
}

-(void) menuQuit {
  hiro::Application::Cocoa::doQuit();
}

-(NSApplicationTerminateReply) applicationShouldTerminate:(NSApplication*)sender {
  using hiro::Application;
  if(Application::state().cocoa.onQuit) Application::Cocoa::doQuit();
  else Application::quit();
  return NSTerminateCancel;
}

-(BOOL) applicationShouldHandleReopen:(NSApplication*)application hasVisibleWindows:(BOOL)flag {
  using hiro::Application;
  if(Application::state().cocoa.onActivate) Application::Cocoa::doActivate();
  return NO;
}

-(void) run:(NSTimer*)timer {
  using hiro::Application;
  if(Application::state().onMain) Application::doMain();
}

-(BOOL)application:(NSApplication*)sender openFile:(NSString*)filename {
  BOOL isDirectory = NO;
  [[NSFileManager defaultManager] fileExistsAtPath:filename isDirectory:&isDirectory];
  if(isDirectory) filename = [filename stringByAppendingString:@"/"];
  hiro::Application::doOpenFile(filename.UTF8String);
  return YES;
}

@end

CocoaDelegate* cocoaDelegate = nullptr;
NSTimer* applicationTimer = nullptr;

namespace hiro {

auto pApplication::exit() -> void {
  quit();
  ::exit(EXIT_SUCCESS);
}

auto pApplication::modal() -> bool {
  return Application::state().modal > 0;
}

auto pApplication::run() -> void {
  if(Application::state().onMain) {
    applicationTimer = [NSTimer scheduledTimerWithTimeInterval:0.0 target:cocoaDelegate selector:@selector(run:) userInfo:nil repeats:YES];

    //below line is needed to run application during window resize; however it has a large performance penalty on the resize smoothness
    //[[NSRunLoop currentRunLoop] addTimer:applicationTimer forMode:NSEventTrackingRunLoopMode];
  }
  [[NSUserDefaults standardUserDefaults] registerDefaults:@{
    //@"NO" is not a mistake; the value really needs to be a string
    @"NSTreatUnknownArgumentsAsOpen": @"NO"
  }];

  @autoreleasepool {
    [NSApp run];
  }
}

auto pApplication::pendingEvents() -> bool {
  bool result = false;
  @autoreleasepool {
    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:NO];
    if(event != nil) result = true;
  }
  return result;
}

auto pApplication::processEvents() -> void {
  @autoreleasepool {
    while(!Application::state().quit) {
      NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:[NSDate distantPast] inMode:NSDefaultRunLoopMode dequeue:YES];
      if(event == nil) break;
      [NSApp sendEvent:event];
    }
  }
}

auto pApplication::quit() -> void {
  @autoreleasepool {
    [applicationTimer invalidate];
    [NSApp stop:nil];
    NSEvent* event = [NSEvent otherEventWithType:NSEventTypeApplicationDefined location:NSMakePoint(0, 0) modifierFlags:0 timestamp:0.0 windowNumber:0 context:nil subtype:0 data1:0 data2:0];
    [NSApp postEvent:event atStart:true];
  }
}

auto pApplication::setScreenSaver(bool screenSaver) -> void {
  static IOPMAssertionID powerAssertion = kIOPMNullAssertionID;  //default is enabled

  //do nothing if state has not been changed
  if(screenSaver == (powerAssertion == kIOPMNullAssertionID)) return;

  if(screenSaver) {
    IOPMAssertionRelease(powerAssertion);
    powerAssertion = kIOPMNullAssertionID;
  } else {
    string reason = {Application::state().name, " screensaver suppression"};
    NSString* assertionName = [NSString stringWithUTF8String:reason.data()];
    if(IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep,
                                   kIOPMAssertionLevelOn, (__bridge CFStringRef)assertionName, &powerAssertion
    ) != kIOReturnSuccess) {
      powerAssertion = kIOPMNullAssertionID;
    }
  }
}

auto pApplication::initialize() -> void {
  @autoreleasepool {
    [NSApplication sharedApplication];
    cocoaDelegate = [[CocoaDelegate alloc] init];
    [NSApp setDelegate:cocoaDelegate];
    [cocoaDelegate constructMenu];
  }
}

}

#endif
