#include <SDL2/SDL.h>

#if defined(PLATFORM_WINDOWS)
#include "shared/rawinput.cpp"
#include "keyboard/rawinput.cpp"
#include "mouse/rawinput.cpp"
#elif defined(PLATFORM_MACOS)
#include "keyboard/quartz.cpp"
#include "mouse/nsmouse.cpp"
#else
#include <sys/ipc.h>
#include <sys/shm.h>
#include "keyboard/xlib.cpp"
#include "mouse/xlib.cpp"
#endif

#include "joypad/sdl.cpp"

struct InputSDL : InputDriver {
  InputSDL& self = *this;
  InputSDL(Input& super) : InputDriver(super), keyboard(super), mouse(super), joypad(super) {}
  ~InputSDL() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "SDL"; }
  auto ready() -> bool override { return isReady; }

  auto hasContext() -> bool override { return true; }

  auto setContext(uintptr context) -> bool override { return initialize(); }

  auto acquired() -> bool override { return mouse.acquired(); }
  auto acquire() -> bool override { return mouse.acquire(); }
  auto release() -> bool override { return mouse.release(); }

  auto poll() -> vector<shared_pointer<HID::Device>> override {
    vector<shared_pointer<HID::Device>> devices;
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

#if defined(PLATFORM_WINDOWS)
    //TODO: this won't work if Input is recreated post-initialization; nor will it work with multiple Input instances
    if(!rawinput.initialized) {
      rawinput.initialized = true;
      rawinput.mutex = CreateMutex(nullptr, false, nullptr);
      CreateThread(nullptr, 0, RawInputThreadProc, 0, 0, nullptr);

      do {
        Sleep(1);
        WaitForSingleObject(rawinput.mutex, INFINITE);
        ReleaseMutex(rawinput.mutex);
      } while(!rawinput.ready);
    }
#endif

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

#if defined(PLATFORM_WINDOWS)
  InputKeyboardRawInput keyboard;
  InputMouseRawInput mouse;
#elif defined(PLATFORM_MACOS)
  InputKeyboardQuartz keyboard;
  InputMouseNS mouse;
#else
  InputKeyboardXlib keyboard;
  InputMouseXlib mouse;
#endif

  InputJoypadSDL joypad;
};
