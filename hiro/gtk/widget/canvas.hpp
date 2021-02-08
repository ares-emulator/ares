#if defined(Hiro_Canvas)

namespace hiro {

struct pCanvas : pWidget {
  Declare(Canvas, Widget)

  auto handle() const -> uintptr;
  auto minimumSize() const -> Size override;
  auto setAlignment(Alignment) -> void;
  auto setColor(Color color) -> void;
  auto setFocusable(bool focusable) -> void override;
  auto setGeometry(Geometry geometry) -> void override;
  auto setGradient(Gradient gradient) -> void;
  auto setIcon(const image& icon) -> void;
  auto update() -> void;

  auto _onDraw(cairo_t* context) -> void;
  auto _onExpose(GdkEventExpose* event) -> void;
  auto _rasterize() -> void;
  auto _redraw() -> void;
  auto _release() -> void;

  GdkPixbuf* surface = nullptr;
  u32 surfaceWidth = 0;
  u32 surfaceHeight = 0;
};

}

#endif
