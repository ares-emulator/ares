#if defined(Hiro_Size)

Size::Size() {
  setSize(0, 0);
}

Size::Size(f32 width, f32 height) {
  setSize(width, height);
}

Size::operator bool() const {
  return state.width || state.height;
}

auto Size::operator==(const Size& source) const -> bool {
  return width() == source.width() && height() == source.height();
}

auto Size::operator!=(const Size& source) const -> bool {
  return !operator==(source);
}

auto Size::height() const -> f32 {
  return state.height;
}

auto Size::reset() -> type& {
  return setSize(0, 0);
}

auto Size::setHeight(f32 height) -> type& {
  state.height = height;
  return *this;
}

auto Size::setSize(Size size) -> type& {
  return setSize(size.width(), size.height());
}

auto Size::setSize(f32 width, f32 height) -> type& {
  state.width = width;
  state.height = height;
  return *this;
}

auto Size::setWidth(f32 width) -> type& {
  state.width = width;
  return *this;
}

auto Size::width() const -> f32 {
  return state.width;
}

#endif
