#if defined(Hiro_Alignment)

const Alignment Alignment::Center = {0.5, 0.5};

Alignment::Alignment() {
  setAlignment(-1.0, -1.0);
}

Alignment::Alignment(f32 horizontal, f32 vertical) {
  setAlignment(horizontal, vertical);
}

Alignment::operator bool() const {
  return state.horizontal >= 0.0 && state.horizontal <= 1.0
      && state.vertical   >= 0.0 && state.vertical   <= 1.0;
}

auto Alignment::horizontal() const -> f32 {
  return state.horizontal;
}

auto Alignment::reset() -> type& {
  return setAlignment(-1.0, -1.0);
}

auto Alignment::setAlignment(f32 horizontal, f32 vertical) -> type& {
  state.horizontal = horizontal;
  state.vertical   = vertical;
  return *this;
}

auto Alignment::setHorizontal(f32 horizontal) -> type& {
  state.horizontal = horizontal;
  return *this;
}

auto Alignment::setVertical(f32 vertical) -> type& {
  state.vertical = vertical;
  return *this;
}

auto Alignment::vertical() const -> f32 {
  return state.vertical;
}

#endif
