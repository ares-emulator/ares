#if defined(Hiro_Timer)

@implementation CocoaTimer : NSObject

-(id) initWith:(hiro::mTimer&)timerReference {
  if(self = [super init]) {
    timer = &timerReference;
    instance = nil;
  }
  return self;
}

-(NSTimer*) instance {
  return instance;
}

-(void) update {
  if(instance) {
    [instance invalidate];
    instance = nil;
  }
  if(timer->enabled()) {
    instance = [NSTimer
      scheduledTimerWithTimeInterval:timer->state.interval / 1000.0
      target:self selector:@selector(run:) userInfo:nil repeats:YES
    ];
  }
}

-(void) run:(NSTimer*)instance {
  if(hiro::Application::state().quit) return;

  if(timer->enabled()) {
    timer->doActivate();
  }
}

@end

namespace hiro {

auto pTimer::construct() -> void {
  cocoaTimer = [[CocoaTimer alloc] initWith:self()];
}

auto pTimer::destruct() -> void {
  if([cocoaTimer instance]) [[cocoaTimer instance] invalidate];
}

auto pTimer::setEnabled(bool enabled) -> void {
  [cocoaTimer update];
}

auto pTimer::setInterval(u32 interval) -> void {
  [cocoaTimer update];
}

}

#endif
