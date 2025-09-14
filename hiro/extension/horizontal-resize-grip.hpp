#if defined(Hiro_HorizontalResizeGrip)

struct HorizontalResizeGrip;
struct mHorizontalResizeGrip;
using sHorizontalResizeGrip = shared_pointer<mHorizontalResizeGrip>;

struct mHorizontalResizeGrip : mCanvas {
  using type = mHorizontalResizeGrip;

  mHorizontalResizeGrip();
  auto doActivate() const -> void;
  auto doResize(s32 offset) const -> void;
  auto onActivate(const std::function<void ()>& callback) -> type&;
  auto onResize(const std::function<void (s32)>& callback) -> type&;

//private:
  struct State {
    std::function<void ()> onActivate;
    std::function<void (s32)> onResize;
    s32 offset = 0;
    Position origin;
    Timer timer;
  } state;
};

#endif
