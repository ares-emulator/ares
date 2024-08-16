#if defined(Hiro_MenuRadioItem)

@implementation CocoaMenuRadioItem

-(id) initWith:(hiro::mMenuRadioItem&)menuRadioItemReference {
  if(self = [super initWithTitle:@"" action:@selector(activate) keyEquivalent:@""]) {
    menuRadioItem = &menuRadioItemReference;

    [self setTarget:self];
  //todo: the default image is a check mark, which is not indicative of radio states.
  //however, NSMenuRadio does not respect macOS' dark theme, and is barely visible.
  //[self setOnStateImage:[NSImage imageNamed:@"NSMenuRadio"]];
  }
  return self;
}

-(void) activate {
  menuRadioItem->setChecked();
  menuRadioItem->doActivate();
}

@end

namespace hiro {

auto pMenuRadioItem::construct() -> void {
  cocoaAction = cocoaMenuRadioItem = [[CocoaMenuRadioItem alloc] initWith:self()];
  pAction::construct();

  if(state().checked) setChecked();
  setText(state().text);
}

auto pMenuRadioItem::destruct() -> void {
}

auto pMenuRadioItem::setChecked() -> void {
  if(auto group = state().group) {
    for(auto& weak : group->state.objects) {
      if(auto object = weak.acquire()) {
        if(auto self = object->self()) {
          if(auto p = dynamic_cast<pMenuRadioItem*>(self)) {
            auto state = this == p ? NSOnState : NSOffState;
            [p->cocoaAction setState:state];
          }
        }
      }
    }
  }
}

auto pMenuRadioItem::setGroup(sGroup group) -> void {
}

auto pMenuRadioItem::setText(const string& text) -> void {
  [cocoaAction setTitle:[NSString stringWithUTF8String:text]];
}

}

#endif
