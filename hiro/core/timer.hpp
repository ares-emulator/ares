#if defined(Hiro_Timer)
struct mTimer : mObject {
  Declare(Timer)

  mTimer();

  auto doActivate() const -> void;
  auto interval() const -> u32;
  auto onActivate(const function<void ()>& callback = {}) -> type&;
  auto setInterval(u32 interval = 0) -> type&;

//private:
  struct State {
    u32 interval = 0;
    function<void ()> onActivate;
  } state;
};
#endif
