#if defined(Hiro_VerticalScrollBar)

namespace hiro {

struct pVerticalScrollBar : pWidget {
  Declare(VerticalScrollBar, Widget)

  auto minimumSize() const -> Size override;
  auto setLength(u32 length) -> void;
  auto setPosition(u32 position) -> void;

  auto _setState() -> void;

  QtVerticalScrollBar* qtVerticalScrollBar = nullptr;
};

}

#endif
