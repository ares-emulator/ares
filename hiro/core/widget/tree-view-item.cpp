#if defined(Hiro_TreeView)

auto mTreeViewItem::allocate() -> pObject* {
  return new pTreeViewItem(*this);
}

auto mTreeViewItem::destruct() -> void {
  for(auto& item : state.items) item->destruct();
  mObject::destruct();
}

//

auto mTreeViewItem::append(sTreeViewItem item) -> type& {
  state.items.push_back(item);
  item->setParent(this, itemCount() - 1);
  signal(append, item);
  return *this;
}

auto mTreeViewItem::backgroundColor(bool recursive) const -> Color {
  if(auto color = state.backgroundColor) return color;
  if(recursive) {
    if(auto parent = parentTreeViewItem()) {
      if(auto color = parent->backgroundColor(true)) return color;
    }
    if(auto parent = parentTreeView()) {
      if(auto color = parent->backgroundColor()) return color;
    }
  }
  return {};
}

auto mTreeViewItem::checkable() const -> bool {
  return state.checkable;
}

auto mTreeViewItem::checked() const -> bool {
  return state.checked;
}

auto mTreeViewItem::collapse(bool recursive) -> type& {
  if(recursive) for(auto& item : state.items) item->collapse(recursive);
  setExpanded(false);
  return *this;
}

auto mTreeViewItem::expand(bool recursive) -> type& {
  setExpanded(true);
  if(recursive) for(auto& item : state.items) item->expand(recursive);
  return *this;
}

auto mTreeViewItem::expanded() const -> bool {
  return state.expanded;
}

auto mTreeViewItem::foregroundColor(bool recursive) const -> Color {
  if(auto color = state.foregroundColor) return color;
  if(recursive) {
    if(auto parent = parentTreeViewItem()) {
      if(auto color = parent->foregroundColor(true)) return color;
    }
    if(auto parent = parentTreeView()) {
      if(auto color = parent->foregroundColor()) return color;
    }
  }
  return {};
}

auto mTreeViewItem::icon() const -> multiFactorImage {
  return state.icon;
}

auto mTreeViewItem::item(const string& path) const -> TreeViewItem {
  if(!path) return {};
  auto paths = path.split("/");
  u32 position = paths.takeLeft().natural();
  if(position >= itemCount()) return {};
  if(!paths) return state.items[position];
  return state.items[position]->item(paths.merge("/"));
}

auto mTreeViewItem::itemCount() const -> u32 {
  return state.items.size();
}

auto mTreeViewItem::items() const -> std::vector<TreeViewItem> {
  std::vector<TreeViewItem> items;
  for(auto& item : state.items) items.push_back(item);
  return items;
}

auto mTreeViewItem::path() const -> string {
  if(auto treeViewItem = parentTreeViewItem()) return {treeViewItem->path(), "/", offset()};
  return {offset()};
}

auto mTreeViewItem::remove() -> type& {
  if(auto treeView = parentTreeView()) treeView->remove(*this);
  if(auto treeViewItem = parentTreeViewItem()) treeViewItem->remove(*this);
  return *this;
}

auto mTreeViewItem::remove(sTreeViewItem item) -> type& {
  signal(remove, item);
  state.items.erase(state.items.begin() + item->offset());
  for(auto n : range(item->offset(), itemCount())) {
    state.items[n]->adjustOffset(-1);
  }
  item->setParent();
  return *this;
}

auto mTreeViewItem::selected() const -> bool {
  if(auto treeView = parentTreeView(true)) {
    return path() == treeView->state.selectedPath;
  }
  return false;
}

auto mTreeViewItem::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mTreeViewItem::setCheckable(bool checkable) -> type& {
  state.checkable = checkable;
  signal(setCheckable, checkable);
  return *this;
}

auto mTreeViewItem::setChecked(bool checked) -> type& {
  state.checked = checked;
  signal(setChecked, checked);
  return *this;
}

auto mTreeViewItem::setExpanded(bool expanded) -> type& {
  state.expanded = expanded;
  signal(setExpanded, expanded);
  return *this;
}

auto mTreeViewItem::setFocused() -> type& {
  signal(setFocused);
  return *this;
}

auto mTreeViewItem::setForegroundColor(Color color) -> type& {
  state.foregroundColor = color;
  signal(setForegroundColor, color);
  return *this;
}

auto mTreeViewItem::setIcon(const multiFactorImage& icon) -> type& {
  state.icon = icon;
  signal(setIcon, icon);
  return *this;
}

auto mTreeViewItem::setParent(mObject* parent, s32 offset) -> type& {
  for(auto it = state.items.rbegin(); it != state.items.rend(); ++it) (*it)->destruct();
  mObject::setParent(parent, offset);
  for(auto& item : state.items) item->setParent(this, item->offset());
  return *this;
}

auto mTreeViewItem::setSelected() -> type& {
  signal(setSelected);
  return *this;
}

auto mTreeViewItem::setText(const string& text) -> type& {
  state.text = text;
  signal(setText, text);
  return *this;
}

auto mTreeViewItem::text() const -> string {
  return state.text;
}

#endif
