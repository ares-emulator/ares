#if defined(Hiro_Action)

namespace hiro {

struct pAction :  pObject {
  Declare(Action, Object)

  auto setEnabled(bool enabled) -> void override;
  auto setVisible(bool visible) -> void override;

  NSMenuItem<CocoaMenu>* cocoaAction = nullptr;
};

}

#endif
