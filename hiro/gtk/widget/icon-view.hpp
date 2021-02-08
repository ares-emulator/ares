#if defined(Hiro_IconView)

namespace hiro {

struct pIconView : pWidget {
  Declare(IconView, Widget)

  auto append(sIconViewItem item) -> void;
  auto remove(sIconViewItem item) -> void;
  auto reset() -> void override;
  auto setBackgroundColor(Color color) -> void;
  auto setBatchable(bool batchable) -> void;
  auto setFlow(Orientation flow) -> void;
  auto setForegroundColor(Color color) -> void;
  auto setGeometry(Geometry geometry) -> void override;
  auto setItemIcon(u32 position, const image& icon) -> void;
  auto setItemSelected(u32 position, bool selected) -> void;
  auto setItemSelected(const vector<s32>& selections) -> void;
  auto setItemSelectedAll() -> void;
  auto setItemSelectedNone() -> void;
  auto setItemText(u32 position, const string& text) -> void;
  auto setOrientation(Orientation orientation) -> void;

  auto _updateSelected() -> void;

  GtkWidget* subWidget = nullptr;
  GtkListStore* store = nullptr;
  vector<u32> currentSelection;
};

}

#endif
