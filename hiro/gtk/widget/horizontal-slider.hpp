#if defined(Hiro_HorizontalSlider)

namespace hiro {

struct pHorizontalSlider : pWidget {
  Declare(HorizontalSlider, Widget)

  auto minimumSize() const -> Size override;
  auto setLength(uint length) -> void;
  auto setPosition(uint position) -> void;
};

}

#endif
