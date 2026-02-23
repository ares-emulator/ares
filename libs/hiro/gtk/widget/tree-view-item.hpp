#if defined(Hiro_TreeView)

namespace hiro {

struct pTreeViewItem : pObject {
  Declare(TreeViewItem, Object)

  auto append(sTreeViewItem item) -> void;
  auto remove(sTreeViewItem item) -> void;
  auto setBackgroundColor(Color color) -> void;
  auto setCheckable(bool checkable) -> void;
  auto setChecked(bool checked) -> void;
  auto setExpanded(bool expanded) -> void;
  auto setFocused() -> void override;
  auto setForegroundColor(Color color) -> void;
  auto setIcon(const image& icon) -> void;
  auto setSelected() -> void;
  auto setText(const string& text) -> void;

  auto _minimumWidth(u32 depth = 0) -> u32;
  auto _parentItem() -> pTreeViewItem*;
  auto _parentWidget() -> pTreeView*;
  auto _updateWidth() -> void;

  GtkTreeIter gtkIter;
  u32 _width = 0;
};

}

#endif
