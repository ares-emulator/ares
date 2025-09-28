#if defined(Hiro_IconView)

auto mIconView::allocate() -> pObject* {
  return new pIconView(*this);
}

auto mIconView::destruct() -> void {
  for(auto& item : state.items) item->destruct();
  mWidget::destruct();
}

//

auto mIconView::append(sIconViewItem item) -> type& {
  state.items.push_back(item);
  item->setParent(this, itemCount() - 1);
  signal(append, item);
  return *this;
}

auto mIconView::backgroundColor() const -> Color {
  return state.backgroundColor;
}

auto mIconView::batchable() const -> bool {
  return state.batchable;
}

auto mIconView::batched() const -> std::vector<IconViewItem> {
  std::vector<IconViewItem> items;
  items.reserve(state.items.size());
  for(auto& item : state.items) {
    if(item->selected()) items.push_back(item);
  }
  return items;
}

auto mIconView::doActivate() const -> void {
  if(state.onActivate) return state.onActivate();
}

auto mIconView::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mIconView::doContext() const -> void {
  if(state.onContext) return state.onContext();
}

auto mIconView::flow() const -> Orientation {
  return state.flow;
}

auto mIconView::foregroundColor() const -> Color {
  return state.foregroundColor;
}

auto mIconView::item(u32 position) const -> IconViewItem {
  if(position < itemCount()) return state.items[position];
  return {};
}

auto mIconView::itemCount() const -> u32 {
  return state.items.size();
}

auto mIconView::items() const -> std::vector<IconViewItem> {
  std::vector<IconViewItem> items;
  items.reserve(state.items.size());
  for(auto& item : state.items) items.push_back(item);
  return items;
}

auto mIconView::onActivate(const std::function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mIconView::onChange(const std::function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mIconView::onContext(const std::function<void ()>& callback) -> type& {
  state.onContext = callback;
  return *this;
}

auto mIconView::orientation() const -> Orientation {
  return state.orientation;
}

auto mIconView::remove(sIconViewItem item) -> type& {
  signal(remove, item);
  state.items.erase(state.items.begin() + item->offset());
  for(auto n : range(item->offset(), itemCount())) {
    state.items[n]->adjustOffset(-1);
  }
  item->setParent();
  return *this;
}

auto mIconView::reset() -> type& {
  signal(reset);
  for(auto& item : state.items) item->setParent();
  state.items.clear();
  return *this;
}

auto mIconView::selected() const -> IconViewItem {
  for(auto& item : state.items) {
    if(item->selected()) return item;
  }
  return {};
}

auto mIconView::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mIconView::setBatchable(bool batchable) -> type& {
  state.batchable = batchable;
  signal(setBatchable, batchable);
  return *this;
}

auto mIconView::setFlow(Orientation flow) -> type& {
  state.flow = flow;
  signal(setFlow, flow);
  return *this;
}

auto mIconView::setForegroundColor(Color color) -> type& {
  state.foregroundColor = color;
  signal(setForegroundColor, color);
  return *this;
}

auto mIconView::setOrientation(Orientation orientation) -> type& {
  state.orientation = orientation;
  signal(setOrientation, orientation);
  return *this;
}

auto mIconView::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& item : state.items | std::views::reverse) item->destruct();
  mObject::setParent(parent, offset);
  for(auto& item : state.items) item->setParent(this, item->offset());
  return *this;
}

auto mIconView::setSelected(const std::vector<s32>& selections) -> type& {
  bool selectAll = (!selections.empty() && selections[0] == ~0);
  for(auto& item : state.items) item->state.selected = selectAll;
  if(selectAll) return signal(setItemSelectedAll), *this;
  if(selections.empty()) return signal(setItemSelectedNone), *this;
  for(auto& position : selections) {
    if(position >= itemCount()) continue;
    state.items[position]->state.selected = true;
  }
  signal(setItemSelected, selections);
  return *this;
}

#endif
