struct Model {
  struct Raster {
    u32 clockDivider;
    u32 scanlines;
    u32 vblankScanline;
    u32 canvasHeight;
    u32 viewportX;
    u32 viewportY;
    u32 viewportWidth;
    u32 viewportHeight;
    u32 outputOffset;
    u32 spriteEvaluationStart;
    u32 spriteEvaluationEnd;
    u32 visibleTop;
    u32 visibleLeft;
    u32 visibleRight;
    f64 aspectX;
    f64 aspectY;
    bool oddFrameCycleSkip;
    bool fixedBackdrop;
    n9 backdropColor;

    auto evaluatesSprites(u32 y) const -> bool;
    auto restrictsOAM(u32 y) const -> bool;
    auto pixelVisible(u32 x, u32 y) const -> bool;
    auto backdrop(n9 color) const -> n9;
  };

  static constexpr u32 PaletteSize = 192;
  using Palette = std::array<u8, PaletteSize>;

  Model(const Raster& raster);
  virtual ~Model() = default;

  virtual auto color(n9 color) const -> n64;
  virtual auto mapWriteRegister(n3 address) const -> n3;
  virtual auto status(n8 data) const -> n8;

  const Raster raster;
};
