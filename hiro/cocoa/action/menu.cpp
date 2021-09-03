#if defined(Hiro_Menu)

@implementation CocoaMenu : NSMenuItem

-(id) initWith:(hiro::mMenu&)menuReference {
  if(self = [super initWithTitle:@"" action:nil keyEquivalent:@""]) {
    menu = &menuReference;

    cocoaMenu = [[NSMenu alloc] initWithTitle:@""];
    [self setSubmenu:cocoaMenu];
  }
  return self;
}

-(NSMenu*) cocoaMenu {
  return cocoaMenu;
}

@end

namespace hiro {

auto pMenu::construct() -> void {
  cocoaAction = cocoaMenu = [[CocoaMenu alloc] initWith:self()];
  pAction::construct();

  setIcon(state().icon);
  setText(state().text);
}

auto pMenu::destruct() -> void {
}

auto pMenu::append(sAction action) -> void {
  if(auto pAction = action->self()) {
    [[cocoaAction cocoaMenu] addItem:pAction->cocoaAction];
  }
}

auto pMenu::remove(sAction action) -> void {
  if(auto pAction = action->self()) {
    [[cocoaAction cocoaMenu] removeItem:pAction->cocoaAction];
  }
}

auto pMenu::setIcon(const multiFactorImage& icon, bool force) -> void {
  if (!force) return;
  u32 size = 16;  //there is no API to retrieve the optimal size
  [cocoaAction setImage:NSMakeImage(icon, size, size)];
}

auto pMenu::setIconForFile(const string& filename) -> void {
  NSImage* icon = [[NSWorkspace sharedWorkspace] iconForFile:@((const char*)filename)];
  icon.size = NSMakeSize(16, 16);
  [cocoaAction setImage:icon];
}

auto pMenu::setText(const string& text) -> void {
  [[cocoaAction cocoaMenu] setTitle:[NSString stringWithUTF8String:text]];
  [cocoaAction setTitle:[NSString stringWithUTF8String:text]];
}

}

#endif
