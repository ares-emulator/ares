#if defined(Hiro_VerticalScrollBar)

namespace hiro {

struct pVerticalScrollBar : pWidget {
  Declare(VerticalScrollBar, Widget)

  auto minimumSize() const -> Size;
  auto setLength(uint length) -> void;
  auto setPosition(uint position) -> void;
};

}

#endif
