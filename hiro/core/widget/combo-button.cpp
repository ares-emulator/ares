#if defined(Hiro_ComboButton)

auto mComboButton::allocate() -> pObject* {
  return new pComboButton(*this);
}

auto mComboButton::destruct() -> void {
  for(auto& item : state.items) item->destruct();
  mWidget::destruct();
}

//

auto mComboButton::append(sComboButtonItem item) -> type& {
  if(state.items.empty()) item->state.selected = true;
  state.items.push_back(item);
  item->setParent(this, itemCount() - 1);
  signal(append, item);
  return *this;
}

auto mComboButton::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mComboButton::item(u32 position) const -> ComboButtonItem {
  if(position < itemCount()) return state.items[position];
  return {};
}

auto mComboButton::itemCount() const -> u32 {
  return state.items.size();
}

auto mComboButton::items() const -> std::vector<ComboButtonItem> {
  std::vector<ComboButtonItem> items;
  for(auto& item : state.items) items.push_back(item);
  return items;
}

auto mComboButton::onChange(const function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mComboButton::remove(sComboButtonItem item) -> type& {
  signal(remove, item);
  state.items.erase(state.items.begin() + item->offset());
  for(u32 n : range(item->offset(), itemCount())) {
    state.items[n]->adjustOffset(-1);
  }
  item->setParent();
  return *this;
}

auto mComboButton::reset() -> type& {
  while(!state.items.empty()) remove(state.items.back());
  return *this;
}

auto mComboButton::selected() const -> ComboButtonItem {
  for(auto& item : state.items) {
    if(item->selected()) return item;
  }
  return {};
}

auto mComboButton::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& item : state.items | std::views::reverse) item->destruct();
  mObject::setParent(parent, offset);
  for(auto& item : state.items) item->setParent(this, item->offset());
  return *this;
}

#endif
