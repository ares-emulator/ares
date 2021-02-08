#if defined(Hiro_Size)
struct Size {
  using type = Size;

  Size();
  Size(f32 width, f32 height);
  template<typename W, typename H>
  Size(W width, H height) : Size((f32)width, (f32)height) {}

  explicit operator bool() const;
  auto operator==(const Size& source) const -> bool;
  auto operator!=(const Size& source) const -> bool;

  auto height() const -> f32;
  auto reset() -> type&;
  auto setHeight(f32 height) -> type&;
  auto setSize(Size source = {}) -> type&;
  auto setSize(f32 width, f32 height) -> type&;
  auto setWidth(f32 width) -> type&;
  auto width() const -> f32;

  static constexpr f32 Maximum = -1.0;
  static constexpr f32 Minimum = +0.0;

//private:
  struct State {
    f32 width;
    f32 height;
  } state;
};
#endif
