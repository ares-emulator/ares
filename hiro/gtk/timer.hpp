#if defined(Hiro_Timer)

namespace hiro {

struct pTimer : pObject {
  Declare(Timer, Object)

  auto setEnabled(bool enabled) -> void override;
  auto setInterval(u32 interval) -> void;
};

}

#endif
