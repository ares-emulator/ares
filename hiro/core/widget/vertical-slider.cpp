#if defined(Hiro_VerticalSlider)

auto mVerticalSlider::allocate() -> pObject* {
  return new pVerticalSlider(*this);
}

//

auto mVerticalSlider::doChange() const -> void {
  if(state.onChange) return state.onChange();
}

auto mVerticalSlider::length() const -> u32 {
  return state.length;
}

auto mVerticalSlider::onChange(const std::function<void ()>& callback) -> type& {
  state.onChange = callback;
  return *this;
}

auto mVerticalSlider::position() const -> u32 {
  return state.position;
}

auto mVerticalSlider::setLength(u32 length) -> type& {
  state.length = length;
  signal(setLength, length);
  return *this;
}

auto mVerticalSlider::setPosition(u32 position) -> type& {
  state.position = position;
  signal(setPosition, position);
  return *this;
}

#endif
