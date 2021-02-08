#if defined(Hiro_Position)

Position::Position() {
  setPosition(0, 0);
}

Position::Position(f32 x, f32 y) {
  setPosition(x, y);
}

Position::operator bool() const {
  return state.x || state.y;
}

auto Position::operator==(const Position& source) const -> bool {
  return x() == source.x() && y() == source.y();
}

auto Position::operator!=(const Position& source) const -> bool {
  return !operator==(source);
}

auto Position::reset() -> type& {
  return setPosition(0, 0);
}

auto Position::setPosition(Position position) -> type& {
  return setPosition(position.x(), position.y());
}

auto Position::setPosition(f32 x, f32 y) -> type& {
  state.x = x;
  state.y = y;
  return *this;
}

auto Position::setX(f32 x) -> type& {
  state.x = x;
  return *this;
}

auto Position::setY(f32 y) -> type& {
  state.y = y;
  return *this;
}

auto Position::x() const -> f32 {
  return state.x;
}

auto Position::y() const -> f32 {
  return state.y;
}

#endif
