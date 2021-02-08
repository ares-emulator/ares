#if defined(Hiro_HorizontalScrollBar)

auto mHorizontalScrollBar::allocate() -> pObject* {
  return new pHorizontalScrollBar(*this);
}

//

auto mHorizontalScrollBar::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mHorizontalScrollBar::length() const -> u32 {
  return state.length;
}

auto mHorizontalScrollBar::onChange(const function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mHorizontalScrollBar::position() const -> u32 {
  return state.position;
}

auto mHorizontalScrollBar::setLength(u32 length) -> type& {
  state.length = length;
  signal(setLength, length);
  return *this;
}

auto mHorizontalScrollBar::setPosition(u32 position) -> type& {
  state.position = position;
  signal(setPosition, position);
  return *this;
}

#endif
