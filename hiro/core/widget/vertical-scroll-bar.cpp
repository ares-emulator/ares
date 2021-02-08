#if defined(Hiro_VerticalScrollBar)

auto mVerticalScrollBar::allocate() -> pObject* {
  return new pVerticalScrollBar(*this);
}

//

auto mVerticalScrollBar::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mVerticalScrollBar::length() const -> u32 {
  return state.length;
}

auto mVerticalScrollBar::onChange(const function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mVerticalScrollBar::position() const -> u32 {
  return state.position;
}

auto mVerticalScrollBar::setLength(u32 length) -> type& {
  state.length = length;
  signal(setLength, length);
  return *this;
}

auto mVerticalScrollBar::setPosition(u32 position) -> type& {
  state.position = position;
  signal(setPosition, position);
  return *this;
}

#endif
