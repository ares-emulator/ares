#if defined(Hiro_MenuSeparator)

namespace hiro {

auto pMenuSeparator::construct() -> void {
  cocoaAction = cocoaSeparator = [NSMenuItem separatorItem];
  pAction::construct();
}

auto pMenuSeparator::destruct() -> void {
}

}

#endif
