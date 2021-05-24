#if defined(Hiro_HorizontalSlider)

namespace hiro {

struct pHorizontalSlider : pWidget {
  Declare(HorizontalSlider, Widget)

  auto minimumSize() const -> Size override;
  auto setLength(u32 length) -> void;
  auto setPosition(u32 position) -> void;

  auto _setState() -> void;

  QtHorizontalSlider* qtHorizontalSlider = nullptr;
};

}

#endif
