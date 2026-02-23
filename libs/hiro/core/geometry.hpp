#if defined(Hiro_Geometry)
struct Geometry {
  using type = Geometry;

  Geometry();
  Geometry(Position position, Size size);
  Geometry(f32 x, f32 y, f32 width, f32 height);
  template<typename X, typename Y, typename W, typename H>
  Geometry(X x, Y y, W width, H height) : Geometry((f32)x, (f32)y, (f32)width, (f32)height) {}

  explicit operator bool() const;
  auto operator==(const Geometry& source) const -> bool;
  auto operator!=(const Geometry& source) const -> bool;

  auto height() const -> f32;
  auto position() const -> Position;
  auto reset() -> type&;
  auto setGeometry(Geometry geometry = {}) -> type&;
  auto setGeometry(Position position, Size size) -> type&;
  auto setGeometry(f32 x, f32 y, f32 width, f32 height) -> type&;
  auto setHeight(f32 height) -> type&;
  auto setPosition(Position position = {}) -> type&;
  auto setPosition(f32 x, f32 y) -> type&;
  auto setSize(Size size = {}) -> type&;
  auto setSize(f32 width, f32 height) -> type&;
  auto setWidth(f32 width) -> type&;
  auto setX(f32 x) -> type&;
  auto setY(f32 y) -> type&;
  auto size() const -> Size;
  auto width() const -> f32;
  auto x() const -> f32;
  auto y() const -> f32;

//private:
  struct State {
    f32 x;
    f32 y;
    f32 width;
    f32 height;
  } state;
};
#endif
