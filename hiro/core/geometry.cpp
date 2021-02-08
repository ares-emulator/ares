#if defined(Hiro_Geometry)

Geometry::Geometry() {
  setGeometry(0, 0, 0, 0);
}

Geometry::Geometry(Position position, Size size) {
  setGeometry(position, size);
}

Geometry::Geometry(f32 x, f32 y, f32 width, f32 height) {
  setGeometry(x, y, width, height);
}

Geometry::operator bool() const {
  return state.x || state.y || state.width || state.height;
}

auto Geometry::operator==(const Geometry& source) const -> bool {
  return x() == source.x() && y() == source.y() && width() == source.width() && height() == source.height();
}

auto Geometry::operator!=(const Geometry& source) const -> bool {
  return !operator==(source);
}

auto Geometry::height() const -> f32 {
  return state.height;
}

auto Geometry::position() const -> Position {
  return {state.x, state.y};
}

auto Geometry::reset() -> type& {
  return setGeometry(0, 0, 0, 0);
}

auto Geometry::setHeight(f32 height) -> type& {
  state.height = height;
  return *this;
}

auto Geometry::setGeometry(Geometry geometry) -> type& {
  return setGeometry(geometry.x(), geometry.y(), geometry.width(), geometry.height());
}

auto Geometry::setGeometry(Position position, Size size) -> type& {
  setGeometry(position.x(), position.y(), size.width(), size.height());
  return *this;
}

auto Geometry::setGeometry(f32 x, f32 y, f32 width, f32 height) -> type& {
  state.x = x;
  state.y = y;
  state.width = width;
  state.height = height;
  return *this;
}

auto Geometry::setPosition(Position position) -> type& {
  return setPosition(position.x(), position.y());
}

auto Geometry::setPosition(f32 x, f32 y) -> type& {
  state.x = x;
  state.y = y;
  return *this;
}

auto Geometry::setSize(Size size) -> type& {
  return setSize(size.width(), size.height());
}

auto Geometry::setSize(f32 width, f32 height) -> type& {
  state.width = width;
  state.height = height;
  return *this;
}

auto Geometry::setWidth(f32 width) -> type& {
  state.width = width;
  return *this;
}

auto Geometry::setX(f32 x) -> type& {
  state.x = x;
  return *this;
}

auto Geometry::setY(f32 y) -> type& {
  state.y = y;
  return *this;
}

auto Geometry::size() const -> Size {
  return {state.width, state.height};
}

auto Geometry::width() const -> f32 {
  return state.width;
}

auto Geometry::x() const -> f32 {
  return state.x;
}

auto Geometry::y() const -> f32 {
  return state.y;
}

#endif
