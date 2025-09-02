#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "keyboard/xlib.cpp"
#include "mouse/xlib.cpp"

struct InputXlib : InputDriver {
  InputXlib& self = *this;
  InputXlib(Input& super) : InputDriver(super), keyboard(super), mouse(super) {}
  ~InputXlib() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "Xlib"; }
  auto ready() -> bool override { return isReady; }

  auto hasContext() -> bool override { return true; }

  auto setContext(uintptr context) -> bool override { return initialize(); }

  auto acquired() -> bool override { return mouse.acquired(); }
  auto acquire() -> bool override { return mouse.acquire(); }
  auto release() -> bool override { return mouse.release(); }

  auto poll() -> std::vector<shared_pointer<HID::Device>> override {
    std::vector<shared_pointer<HID::Device>> devices;
    keyboard.poll(devices);
    mouse.poll(devices);
    return devices;
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool override {
    return false;
  }

private:
  auto initialize() -> bool {
    terminate();
    if(!self.context) return false;
    if(!keyboard.initialize()) return false;
    if(!mouse.initialize(self.context)) return false;
    return isReady = true;
  }

  auto terminate() -> void {
    isReady = false;
    keyboard.terminate();
    mouse.terminate();
  }

  bool isReady = false;
  InputKeyboardXlib keyboard;
  InputMouseXlib mouse;
};
