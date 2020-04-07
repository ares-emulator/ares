#if defined(Hiro_TabFrame)

namespace hiro {

struct pTabFrame : pWidget {
  Declare(TabFrame, Widget)

  auto append(sTabFrameItem item) -> void;
  auto container(mWidget& widget) -> GtkWidget* override;
  auto remove(sTabFrameItem item) -> void;
  auto setFont(const Font& font) -> void override;
  auto setGeometry(Geometry geometry) -> void override;
  auto setItemClosable(uint position, bool closable) -> void;
  auto setItemIcon(uint position, const image& icon) -> void;
  auto setItemMovable(uint position, bool movable) -> void;
  auto setItemSelected(uint position) -> void;
  auto setItemSizable(uint position, sSizable sizable) -> void;
  auto setItemText(uint position, const string& text) -> void;
  auto setNavigation(Navigation navigation) -> void;

  auto _append() -> void;
  auto _synchronizeLayout() -> void;
  auto _synchronizeTab(uint position) -> void;
  auto _tabHeight() -> uint;
  auto _tabWidth() -> uint;

  struct Tab {
    GtkWidget* child = nullptr;
    GtkWidget* container = nullptr;
    GtkWidget* layout = nullptr;
    GtkWidget* image = nullptr;
    GtkWidget* title = nullptr;
    GtkWidget* close = nullptr;
  };
  vector<Tab> tabs;
};

}

#endif
