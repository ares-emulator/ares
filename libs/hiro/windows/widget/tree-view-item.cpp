#if defined(Hiro_TreeView)

namespace hiro {

auto pTreeViewItem::construct() -> void {
  if(auto parentWidget = _parentWidget()) {
    parentWidget->lock();
    auto parentItem = _parentItem();
    wchar_t wtext[] = L"";
    TVINSERTSTRUCT tvItem;
    tvItem.hParent = !parentItem ? TVI_ROOT : parentItem->hTreeItem;
    tvItem.hInsertAfter = TVI_LAST;
    tvItem.item.pszText = wtext;
    tvItem.item.cchTextMax = PATH_MAX + 1;
    tvItem.item.mask = TVIF_TEXT;
    hTreeItem = (HTREEITEM)SendMessage(parentWidget->hwnd, TVM_INSERTITEM, 0, (LPARAM)&tvItem);
    parentWidget->unlock();
  }
  setBackgroundColor(state().backgroundColor);
  setCheckable(state().checkable);
  setChecked(state().checked);
  setExpanded(state().expanded);
  setForegroundColor(state().foregroundColor);
  setIcon(state().icon);
  setText(state().text);
}

auto pTreeViewItem::destruct() -> void {
  if(auto parentWidget = _parentWidget()) {
    parentWidget->lock();
    SendMessage(parentWidget->hwnd, TVM_DELETEITEM, 0, (LPARAM)hTreeItem);
    parentWidget->unlock();
  }
}

auto pTreeViewItem::append(sTreeViewItem item) -> void {
}

auto pTreeViewItem::remove(sTreeViewItem item) -> void {
}

auto pTreeViewItem::setBackgroundColor(Color color) -> void {
}

auto pTreeViewItem::setCheckable(bool checkable) -> void {
}

auto pTreeViewItem::setChecked(bool checked) -> void {
}

auto pTreeViewItem::setExpanded(bool expanded) -> void {
  if(auto parentWidget = _parentWidget()) {
    parentWidget->lock();
    SendMessage(parentWidget->hwnd, TVM_EXPAND, expanded ? TVE_EXPAND : TVE_COLLAPSE, (LPARAM)hTreeItem);
    parentWidget->unlock();
  }
}

auto pTreeViewItem::setFocused() -> void {
}

auto pTreeViewItem::setForegroundColor(Color color) -> void {
}

auto pTreeViewItem::setIcon(const image& icon) -> void {
}

auto pTreeViewItem::setSelected() -> void {
  if(auto parentWidget = _parentWidget()) {
    parentWidget->lock();
    SendMessage(parentWidget->hwnd, TVM_SELECTITEM, TVGN_CARET, (LPARAM)&hTreeItem);
    parentWidget->unlock();
  }
}

auto pTreeViewItem::setText(const string& text) -> void {
  if(auto parentWidget = _parentWidget()) {
    parentWidget->lock();
    utf16_t wtext{text};
    TVITEMW tvItem{};
    tvItem.hItem = hTreeItem;
    tvItem.cchTextMax = PATH_MAX + 1;
    tvItem.pszText = wtext;
    tvItem.mask = TVIF_TEXT;
    SendMessage(parentWidget->hwnd, TVM_SETITEM, 0, (LPARAM)&tvItem);
    parentWidget->unlock();
  }
}

//

auto pTreeViewItem::_parentItem() -> pTreeViewItem* {
  if(auto parentItem = self().parentTreeViewItem()) return parentItem->self();
  return nullptr;
}

auto pTreeViewItem::_parentWidget() -> pTreeView* {
  if(auto parentWidget = self().parentTreeView(true)) return parentWidget->self();
  return nullptr;
}

}

#endif
