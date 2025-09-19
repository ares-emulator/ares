#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <libudev.h>
#ifdef __FreeBSD__
#include <dev/evdev/input.h>
#else
#include <linux/types.h>
#include <linux/input.h>
#endif

#include "keyboard/xlib.cpp"
#include "mouse/xlib.cpp"
#include "joypad/udev.cpp"

struct InputUdev : InputDriver {
  InputUdev& self = *this;
  InputUdev(Input& super) : InputDriver(super), keyboard(super), mouse(super), joypad(super) {}
  ~InputUdev() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "udev"; }
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
    joypad.poll(devices);
    return devices;
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool override {
    return joypad.rumble(id, strong, weak);
  }

private:
  auto initialize() -> bool {
    terminate();
    if(!self.context) return false;
    if(!keyboard.initialize()) return false;
    if(!mouse.initialize(self.context)) return false;
    if(!joypad.initialize()) return false;
    return isReady = true;
  }

  auto terminate() -> void {
    isReady = false;
    keyboard.terminate();
    mouse.terminate();
    joypad.terminate();
  }

  bool isReady = false;
  InputKeyboardXlib keyboard;
  InputMouseXlib mouse;
  InputJoypadUdev joypad;
};
