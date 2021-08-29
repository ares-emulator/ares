#if defined(Hiro_MenuItem)

@implementation CocoaMenuItem : NSMenuItem

-(id) initWith:(hiro::mMenuItem&)menuItemReference {
  if(self = [super initWithTitle:@"" action:@selector(activate) keyEquivalent:@""]) {
    menuItem = &menuItemReference;

    [self setTarget:self];
  }
  return self;
}

-(void) activate {
  menuItem->doActivate();
}

@end

namespace hiro {

auto pMenuItem::construct() -> void {
  cocoaAction = cocoaMenuItem = [[CocoaMenuItem alloc] initWith:self()];
  pAction::construct();

  setIcon(state().icon);
  setText(state().text);
}

auto pMenuItem::destruct() -> void {
}

auto pMenuItem::setIcon(const image& icon, bool force) -> void {
  if(!force) return;
  u32 size = 16;  //there is no API to retrieve the optimal size
  [cocoaAction setImage:NSMakeImage(icon, size, size)];
}

auto pMenuItem::setText(const string& text) -> void {
  [cocoaAction setTitle:[NSString stringWithUTF8String:text]];
}

}

#endif
