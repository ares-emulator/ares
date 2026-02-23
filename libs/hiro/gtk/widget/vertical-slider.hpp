#if defined(Hiro_VerticalSlider)

namespace hiro {

struct pVerticalSlider : pWidget {
  Declare(VerticalSlider, Widget)

  auto minimumSize() const -> Size override;
  auto setLength(u32 length) -> void;
  auto setPosition(u32 position) -> void;
};

}

#endif
