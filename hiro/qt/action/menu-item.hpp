#if defined(Hiro_MenuItem)

namespace hiro {

struct pMenuItem : pAction {
  Declare(MenuItem, Action)

  auto setIcon(const image& icon, bool force = false) -> void;
  auto setText(const string& text) -> void;

  auto _setState() -> void override;

  QtMenuItem* qtMenuItem = nullptr;
};

}

#endif
