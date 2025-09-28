#if defined(Hiro_VerticalResizeGrip)

mVerticalResizeGrip::mVerticalResizeGrip() {
  image icon;
  icon.allocate(15, 5);
  for(u32 x : range(icon.width())) {
    auto data = icon.data() + x * icon.stride();
    icon.write(data, 0x00000000); data += icon.pitch();
    icon.write(data, 0xff9f9f9f); data += icon.pitch();
    icon.write(data, 0x00000000); data += icon.pitch();
    icon.write(data, 0xff9f9f9f); data += icon.pitch();
    icon.write(data, 0x00000000); data += icon.pitch();
  }
  mCanvas::setIcon(icon);
  mCanvas::setMouseCursor(MouseCursor::VerticalResize);
  mCanvas::onMousePress([&](auto button) {
    if(button == Mouse::Button::Left && !state.timer.enabled()) {
      doActivate();
      state.offset = 0;
      state.origin = Mouse::position();
      state.timer.setEnabled();
    }
  });
  state.timer.setInterval(10).onActivate([&] {
    if(!Mouse::pressed(Mouse::Button::Left)) return (void)state.timer.setEnabled(false);
    auto position = Mouse::position();
    auto offset = position.y() - state.origin.y();
    if(offset != state.offset) doResize(offset), offset = state.offset;
  });
}

auto mVerticalResizeGrip::doActivate() const -> void {
  if(state.onActivate) state.onActivate();
}

auto mVerticalResizeGrip::doResize(s32 offset) const -> void {
  if(state.onResize) state.onResize(offset);
}

auto mVerticalResizeGrip::onActivate(const std::function<void ()>& callback) -> type& {
  state.onActivate = callback;
  return *this;
}

auto mVerticalResizeGrip::onResize(const std::function<void (s32)>& callback) -> type& {
  state.onResize = callback;
  return *this;
}

#endif
