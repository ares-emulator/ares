#if defined(Hiro_Timer)

//timeBeginPeriod(1) + timeSetEvent does not seem any more performant than SetTimer
//it also seems buggier, and requires libwinmm

namespace hiro {

static std::vector<pTimer*> timers;

static auto CALLBACK Timer_timeoutProc(HWND hwnd, UINT msg, UINT_PTR timerID, DWORD time) -> void {
  if(Application::state().quit) return;

  for(auto& timer : timers) {
    if(timer->htimer == timerID) return timer->self().doActivate();
  }
}

auto pTimer::construct() -> void {
  timers.push_back(this);
  htimer = 0;
}

auto pTimer::destruct() -> void {
  setEnabled(false);
  if(auto it = std::find(timers.begin(), timers.end(), this); it != timers.end()) timers.erase(it);
}

auto pTimer::setEnabled(bool enabled) -> void {
  if(htimer) {
    KillTimer(nullptr, htimer);
    htimer = 0;
  }

  if(enabled == true) {
    htimer = SetTimer(nullptr, 0, state().interval, Timer_timeoutProc);
  }
}

auto pTimer::setInterval(u32 interval) -> void {
  //destroy and recreate timer if interval changed
  setEnabled(self().enabled(true));
}

}

#endif
