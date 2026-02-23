#if defined(Hiro_Position)
struct Position {
  using type = Position;

  Position();
  Position(f32 x, f32 y);
  template<typename X, typename Y>
  Position(X x, Y y) : Position((f32)x, (f32)y) {}

  explicit operator bool() const;
  auto operator==(const Position& source) const -> bool;
  auto operator!=(const Position& source) const -> bool;

  auto reset() -> type&;
  auto setPosition(Position position = {}) -> type&;
  auto setPosition(f32 x, f32 y) -> type&;
  auto setX(f32 x) -> type&;
  auto setY(f32 y) -> type&;
  auto x() const -> f32;
  auto y() const -> f32;

//private:
  struct State {
    f32 x;
    f32 y;
  } state;
};
#endif
