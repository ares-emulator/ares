#if defined(Hiro_MenuSeparator)

namespace hiro {

auto pMenuSeparator::construct() -> void {
  @autoreleasepool {
    cocoaAction = cocoaSeparator = [NSMenuItem separatorItem];
    pAction::construct();
  }
}

auto pMenuSeparator::destruct() -> void {
}

}

#endif
