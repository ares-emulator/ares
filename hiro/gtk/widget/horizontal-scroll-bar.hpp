#if defined(Hiro_HorizontalScrollBar)

namespace hiro {

struct pHorizontalScrollBar : pWidget {
  Declare(HorizontalScrollBar, Widget)

  auto minimumSize() const -> Size;
  auto setLength(uint length) -> void;
  auto setPosition(uint position) -> void;
};

}

#endif
