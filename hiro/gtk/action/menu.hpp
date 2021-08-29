#if defined(Hiro_Menu)

namespace hiro {

struct pMenu : pAction {
  Declare(Menu, Action)

  auto append(sAction action) -> void;
  auto remove(sAction action) -> void;
  auto setFont(const Font& font) -> void override;
  auto setIcon(const image& icon, bool force = false) -> void;
  auto setText(const string& text) -> void;

  GtkWidget* gtkMenu = nullptr;
};

}

#endif
