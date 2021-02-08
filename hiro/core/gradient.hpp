#if defined(Hiro_Gradient)
struct Gradient {
  using type = Gradient;

  Gradient();

  explicit operator bool() const;
  auto operator==(const Gradient& source) const -> bool;
  auto operator!=(const Gradient& source) const -> bool;

  auto setBilinear(Color topLeft, Color topRight, Color bottomLeft, Color bottomRight) -> type&;
  auto setHorizontal(Color left, Color right) -> type&;
  auto setVertical(Color top, Color bottom) -> type&;

//private:
  struct State {
    vector<Color> colors;
  } state;
};
#endif
