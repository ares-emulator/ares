#if defined(Hiro_HorizontalSlider)
struct mHorizontalSlider : mWidget {
  Declare(HorizontalSlider)

  auto doChange() const -> void;
  auto length() const -> u32;
  auto onChange(const function<void ()>& callback = {}) -> type&;
  auto position() const -> u32;
  auto setLength(u32 length = 101) -> type&;
  auto setPosition(u32 position = 0) -> type&;

//private:
  struct State {
    u32 length = 101;
    function<void ()> onChange;
    u32 position = 0;
  } state;
};
#endif
