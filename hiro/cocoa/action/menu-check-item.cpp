#if defined(Hiro_MenuCheckItem)

@implementation CocoaMenuCheckItem : NSMenuItem

-(id) initWith:(hiro::mMenuCheckItem&)menuCheckItemReference {
  if(self = [super initWithTitle:@"" action:@selector(activate) keyEquivalent:@""]) {
    menuCheckItem = &menuCheckItemReference;

    [self setTarget:self];
  }
  return self;
}

-(void) activate {
  menuCheckItem->state.checked = !menuCheckItem->state.checked;
  auto state = menuCheckItem->state.checked ? NSOnState : NSOffState;
  [self setState:state];
  menuCheckItem->doToggle();
}

@end

namespace hiro {

auto pMenuCheckItem::construct() -> void {
  cocoaAction = cocoaMenuCheckItem = [[CocoaMenuCheckItem alloc] initWith:self()];
  pAction::construct();

  setChecked(state().checked);
  setText(state().text);
}

auto pMenuCheckItem::destruct() -> void {
}

auto pMenuCheckItem::setChecked(bool checked) -> void {
  auto state = checked ? NSOnState : NSOffState;
  [cocoaAction setState:state];
}

auto pMenuCheckItem::setText(const string& text) -> void {
  [cocoaAction setTitle:[NSString stringWithUTF8String:text]];
}

}

#endif
