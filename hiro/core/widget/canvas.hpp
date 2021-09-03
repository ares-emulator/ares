#if defined(Hiro_Canvas)
struct mCanvas : mWidget {
  Declare(Canvas)

  auto alignment() const -> Alignment;
  auto color() const -> Color;
  auto data() -> u32*;
  auto gradient() const -> Gradient;
  auto icon() const -> multiFactorImage;
  auto setAlignment(Alignment alignment = {}) -> type&;
  auto setColor(Color color = {}) -> type&;
  auto setGradient(Gradient gradient = {}) -> type&;
  auto setIcon(const multiFactorImage& icon = {}) -> type&;
  auto setSize(Size size = {}) -> type&;
  auto size() const -> Size;
  auto update() -> type&;

//private:
  struct State {
    Alignment alignment;
    Color color;
    Gradient gradient;
    multiFactorImage icon;
  } state;
};
#endif
