#if defined(Hiro_TabFrame)

namespace hiro {

struct pTabFrame : pWidget {
  Declare(TabFrame, Widget)

  auto append(sTabFrameItem item) -> void;
  auto container(mWidget& widget) -> GtkWidget* override;
  auto remove(sTabFrameItem item) -> void;
  auto setFont(const Font& font) -> void override;
  auto setGeometry(Geometry geometry) -> void override;
  auto setItemClosable(u32 position, bool closable) -> void;
  auto setItemIcon(u32 position, const image& icon) -> void;
  auto setItemMovable(u32 position, bool movable) -> void;
  auto setItemSelected(u32 position) -> void;
  auto setItemSizable(u32 position, sSizable sizable) -> void;
  auto setItemText(u32 position, const string& text) -> void;
  auto setNavigation(Navigation navigation) -> void;

  auto _append() -> void;
  auto _synchronizeLayout() -> void;
  auto _synchronizeTab(u32 position) -> void;
  auto _tabHeight() -> u32;
  auto _tabWidth() -> u32;

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
