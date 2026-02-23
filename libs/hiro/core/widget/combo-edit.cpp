#if defined(Hiro_ComboEdit)

auto mComboEdit::allocate() -> pObject* {
  return new pComboEdit(*this);
}

auto mComboEdit::destruct() -> void {
  for(auto& item : state.items) item->destruct();
  mWidget::destruct();
}

//

auto mComboEdit::append(sComboEditItem item) -> type& {
  state.items.push_back(item);
  item->setParent(this, itemCount() - 1);
  signal(append, item);
  return *this;
}

auto mComboEdit::backgroundColor() const -> Color {
  return state.backgroundColor;
}

auto mComboEdit::doActivate() const -> void {
  if(state.onActivate) return state.onActivate();
}

auto mComboEdit::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mComboEdit::editable() const -> bool {
  return state.editable;
}

auto mComboEdit::foregroundColor() const -> Color {
  return state.foregroundColor;
}

auto mComboEdit::item(u32 position) const -> ComboEditItem {
  if(position < itemCount()) return state.items[position];
  return {};
}

auto mComboEdit::itemCount() const -> u32 {
  return state.items.size();
}

auto mComboEdit::items() const -> std::vector<ComboEditItem> {
  std::vector<ComboEditItem> items;
  items.reserve(state.items.size());
  for(auto& item : state.items) items.push_back(item);
  return items;
}

auto mComboEdit::onActivate(const std::function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mComboEdit::onChange(const std::function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mComboEdit::remove(sComboEditItem item) -> type& {
  signal(remove, item);
  state.items.erase(state.items.begin() + item->offset());
  for(auto n : range(item->offset(), itemCount())) {
    state.items[n]->adjustOffset(-1);
  }
  item->setParent();
  return *this;
}

auto mComboEdit::reset() -> type& {
  signal(reset);
  for(auto& item : state.items) item->setParent();
  state.items.clear();
  return *this;
}

auto mComboEdit::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mComboEdit::setEditable(bool editable) -> type& {
  state.editable = editable;
  signal(setEditable, editable);
  return *this;
}

auto mComboEdit::setForegroundColor(Color color) -> type& {
  state.foregroundColor = color;
  signal(setForegroundColor, color);
  return *this;
}

auto mComboEdit::setParent(mObject* parent, s32 offset) -> type& {
  for(auto& item : state.items) item->destruct();
  mObject::setParent(parent, offset);
  for(auto& item : state.items) item->setParent(this, item->offset());
  return *this;
}

auto mComboEdit::setText(const string& text) -> type& {
  state.text = text;
  signal(setText, text);
  return *this;
}

auto mComboEdit::text() const -> string {
  return state.text;
}

#endif
