#if defined(Hiro_TreeView)

auto mTreeView::allocate() -> pObject* {
  return new pTreeView(*this);
}

auto mTreeView::destruct() -> void {
  for(auto& item : state.items) item->destruct();
  mWidget::destruct();
}

//

auto mTreeView::activation() const -> Mouse::Click {
  return state.activation;
}

auto mTreeView::append(sTreeViewItem item) -> type& {
  state.items.push_back(item);
  item->setParent(this, itemCount() - 1);
  signal(append, item);
  return *this;
}

auto mTreeView::backgroundColor() const -> Color {
  return state.backgroundColor;
}

auto mTreeView::collapse(bool recursive) -> type& {
  for(auto& item : state.items) item->collapse(recursive);
  return *this;
}

auto mTreeView::doActivate() const -> void {
  if(state.onActivate) return state.onActivate();
}

auto mTreeView::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mTreeView::doContext() const -> void {
  if(state.onContext) return state.onContext();
}

auto mTreeView::doToggle(sTreeViewItem item) const -> void {
  if(state.onToggle) return state.onToggle(item);
}

auto mTreeView::expand(bool recursive) -> type& {
  for(auto& item : state.items) item->expand(recursive);
  return *this;
}

auto mTreeView::foregroundColor() const -> Color {
  return state.foregroundColor;
}

auto mTreeView::item(const string& path) const -> TreeViewItem {
  if(!path) return {};
  auto paths = path.split("/");
  u32 position = paths.takeLeft().natural();
  if(position >= itemCount()) return {};
  if(!paths) return state.items[position];
  return state.items[position]->item(paths.merge("/"));
}

auto mTreeView::itemCount() const -> u32 {
  return state.items.size();
}

auto mTreeView::items() const -> std::vector<TreeViewItem> {
  std::vector<TreeViewItem> items;
  for(auto& item : state.items) items.push_back(item);
  return items;
}

auto mTreeView::onActivate(const function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mTreeView::onChange(const function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mTreeView::onContext(const function<void ()>& callback) -> type& {
  state.onContext = callback;
  return *this;
}

auto mTreeView::onToggle(const function<void (sTreeViewItem)>& callback) -> type& {
  state.onToggle = callback;
  return *this;
}

auto mTreeView::remove(sTreeViewItem item) -> type& {
  signal(remove, item);
  state.items.erase(state.items.begin() + item->offset());
  for(auto n : range(item->offset(), itemCount())) {
    state.items[n]->adjustOffset(-1);
  }
  item->setParent();
  return *this;
}

auto mTreeView::reset() -> type& {
  state.selectedPath.reset();
  while(!state.items.empty()) remove(state.items.back());
  return *this;
}

auto mTreeView::selectNone() -> type& {
  if(auto item = selected()) {
  //TODO
  //item->setSelected(false);
  }
  return *this;
}

auto mTreeView::selected() const -> TreeViewItem {
  return item(state.selectedPath);
}

auto mTreeView::setActivation(Mouse::Click activation) -> type& {
  state.activation = activation;
  signal(setActivation, activation);
  return *this;
}

auto mTreeView::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mTreeView::setForegroundColor(Color color) -> type& {
  state.foregroundColor = color;
  signal(setForegroundColor, color);
  return *this;
}

auto mTreeView::setParent(mObject* object, s32 offset) -> type& {
  for(auto& item : state.items | std::views::reverse) item->destruct();
  mObject::setParent(object, offset);
  for(auto& item : state.items) item->setParent(this, item->offset());
  return *this;
}

#endif
