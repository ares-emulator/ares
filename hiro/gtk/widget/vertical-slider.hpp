#if defined(Hiro_VerticalSlider)

namespace hiro {

struct pVerticalSlider : pWidget {
  Declare(VerticalSlider, Widget)

  auto minimumSize() const -> Size override;
  auto setLength(uint length) -> void;
  auto setPosition(uint position) -> void;
};

}

#endif
