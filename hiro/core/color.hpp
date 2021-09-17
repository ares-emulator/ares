#if defined(Hiro_Color)
struct Color {
  using type = Color;

  Color();
  Color(s32 red, s32 green, s32 blue, s32 alpha = 255);
  Color(SystemColor color);

  explicit operator bool() const;
  auto operator==(const Color& source) const -> bool;
  auto operator!=(const Color& source) const -> bool;

  auto alpha() const -> u8;
  auto blue() const -> u8;
  auto green() const -> u8;
  auto red() const -> u8;
  auto reset() -> type&;
  auto setAlpha(s32 alpha) -> type&;
  auto setBlue(s32 blue) -> type&;
  auto setColor(Color color = {}) -> type&;
  auto setColor(s32 red, s32 green, s32 blue, s32 alpha = 255) -> type&;
  auto setGreen(s32 green) -> type&;
  auto setRed(s32 red) -> type&;
  auto setValue(u32 value) -> type&;
  auto value() const -> u32;

//private:
  struct State {
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
  } state;
};
#endif
