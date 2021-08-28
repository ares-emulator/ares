#if defined(Hiro_Console)

@implementation CocoaConsole : NSScrollView

-(id) initWith:(phoenix::Console&)consoleReference {
  if(self = [super initWithFrame:NSMakeRect(0, 0, 0, 0)]) {
    console = &consoleReference;
  }
  return self;
}

@end

namespace hiro {

auto pConsole::print(string text) -> void {
}

auto pConsole::reset() -> void {
}

auto pConsole::setBackgroundColor(Color color) -> void {
}

auto pConsole::setForegroundColor(Color color) -> void {
}

auto pConsole::setPrompt(string prompt) -> void {
}

auto pConsole::constructor() -> void {
  cocoaView = cocoaConsole = [[CocoaConsole alloc] initWith:console];
}

auto pConsole::destructor() -> void {
  [cocoaView removeFromSuperview];
  [cocoaView release];
}

}

#endif
