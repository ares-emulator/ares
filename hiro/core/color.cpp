#if defined(Hiro_Color)

Color::Color() {
  setColor(0, 0, 0, 0);
}

Color::Color(s32 red, s32 green, s32 blue, s32 alpha) {
  setColor(red, green, blue, alpha);
}

Color::Color(SystemColor color) {
  switch (color) {
    case SystemColor::Text: setColor(0, 0, 0, 255); return;
    case SystemColor::Label: setColor(0, 0, 0, 255); return;
    case SystemColor::Sublabel: setColor(80, 80, 80, 255); return;
    case SystemColor::Link: setColor(0, 85, 255, 255); return;
    case SystemColor::PlaceholderText: setColor(128, 128, 128, 255); return;
    default: setColor(0, 0, 0, 0); return;
  }
}

Color::operator bool() const {
  return state.red || state.green || state.blue || state.alpha;
}

auto Color::operator==(const Color& source) const -> bool {
  return red() == source.red() && green() == source.green() && blue() == source.blue() && alpha() == source.alpha();
}

auto Color::operator!=(const Color& source) const -> bool {
  return !operator==(source);
}

auto Color::alpha() const -> u8 {
  return state.alpha;
}

auto Color::blue() const -> u8 {
  return state.blue;
}

auto Color::green() const -> u8 {
  return state.green;
}

auto Color::red() const -> u8 {
  return state.red;
}

auto Color::reset() -> type& {
  return setColor(0, 0, 0, 0);
}

auto Color::setAlpha(s32 alpha) -> type& {
  state.alpha = max(0, min(255, alpha));
  return *this;
}

auto Color::setBlue(s32 blue) -> type& {
  state.blue = max(0, min(255, blue));
  return *this;
}

auto Color::setColor(Color color) -> type& {
  return setColor(color.red(), color.green(), color.blue(), color.alpha());
}

auto Color::setColor(s32 red, s32 green, s32 blue, s32 alpha) -> type& {
  state.red   = max(0, min(255, red  ));
  state.green = max(0, min(255, green));
  state.blue  = max(0, min(255, blue ));
  state.alpha = max(0, min(255, alpha));
  return *this;
}

auto Color::setGreen(s32 green) -> type& {
  state.green = max(0, min(255, green));
  return *this;
}

auto Color::setRed(s32 red) -> type& {
  state.red = max(0, min(255, red));
  return *this;
}

auto Color::setValue(u32 value) -> type& {
  state.alpha = value >> 24 & 255;
  state.red   = value >> 16 & 255;
  state.green = value >>  8 & 255;
  state.blue  = value >>  0 & 255;
  return *this;
}

auto Color::value() const -> u32 {
  return state.alpha << 24 | state.red << 16 | state.green << 8 | state.blue << 0;
}

#endif
