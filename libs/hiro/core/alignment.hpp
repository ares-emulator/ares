#if defined(Hiro_Alignment)
struct Alignment {
  using type = Alignment;
  static const Alignment Center;

  Alignment();
  Alignment(f32 horizontal, f32 vertical = 0.5);

  explicit operator bool() const;
  auto operator==(const Alignment& source) const -> bool;
  auto operator!=(const Alignment& source) const -> bool;

  auto horizontal() const -> f32;
  auto reset() -> type&;
  auto setAlignment(f32 horizontal = -1.0, f32 vertical = 0.5) -> type&;
  auto setHorizontal(f32 horizontal) -> type&;
  auto setVertical(f32 vertical) -> type&;
  auto vertical() const -> f32;

//private:
  struct State {
    f32 horizontal;
    f32 vertical;
  } state;
};
#endif
