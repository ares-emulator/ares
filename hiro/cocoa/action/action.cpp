#if defined(Hiro_Action)

namespace hiro {

auto pAction::construct() -> void {
}

auto pAction::destruct() -> void {
}

auto pAction::setEnabled(bool enabled) -> void {
  [cocoaAction setEnabled:enabled];
}

auto pAction::setVisible(bool visible) -> void {
  [cocoaAction setHidden:!visible];
}

}

#endif
