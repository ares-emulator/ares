#if defined(Hiro_VerticalResizeGrip)

struct VerticalResizeGrip;
struct mVerticalResizeGrip;
using sVerticalResizeGrip = shared_pointer<mVerticalResizeGrip>;

struct mVerticalResizeGrip : mCanvas {
  using type = mVerticalResizeGrip;

  mVerticalResizeGrip();
  auto doActivate() const -> void;
  auto doResize(s32 offset) const -> void;
  auto onActivate(const function<void ()>& callback) -> type&;
  auto onResize(const function<void (s32)>& callback) -> type&;

//private:
  struct State {
    function<void ()> onActivate;
    function<void (s32)> onResize;
    s32 offset = 0;
    Position origin;
    Timer timer;
  } state;
};

#endif
