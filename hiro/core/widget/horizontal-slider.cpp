#if defined(Hiro_HorizontalSlider)

auto mHorizontalSlider::allocate() -> pObject* {
  return new pHorizontalSlider(*this);
}

//

auto mHorizontalSlider::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mHorizontalSlider::length() const -> u32 {
  return state.length;
}

auto mHorizontalSlider::onChange(const function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mHorizontalSlider::position() const -> u32 {
  return state.position;
}

auto mHorizontalSlider::setLength(u32 length) -> type& {
  state.length = length;
  signal(setLength, length);
  return *this;
}

auto mHorizontalSlider::setPosition(u32 position) -> type& {
  state.position = position;
  signal(setPosition, position);
  return *this;
}

#endif
