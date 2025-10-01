#include <xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "shared/rawinput.cpp"
#include "keyboard/rawinput.cpp"
#include "mouse/rawinput.cpp"
#include "joypad/xinput.cpp"
#include "joypad/directinput.cpp"

struct InputWindows : InputDriver {
  InputWindows& self = *this;
  InputWindows(Input& super) : InputDriver(super), keyboard(super), mouse(super), joypadXInput(super), joypadDirectInput(super) {}
  ~InputWindows() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "Windows"; }
  auto ready() -> bool override { return isReady; }

  auto hasContext() -> bool override { return true; }

  auto setContext(uintptr context) -> bool override { return initialize(); }

  auto acquired() -> bool override { return mouse.acquired(); }
  auto acquire() -> bool override { return mouse.acquire(); }
  auto release() -> bool override { return mouse.release(); }

  auto poll() -> std::vector<std::shared_ptr<HID::Device>> override {
    std::vector<std::shared_ptr<HID::Device>> devices;
    keyboard.poll(devices);
    mouse.poll(devices);
    joypadXInput.poll(devices);
    joypadDirectInput.poll(devices);
    return devices;
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool override {
    if(joypadXInput.rumble(id, strong, weak)) return true;
    if(joypadDirectInput.rumble(id, strong, weak)) return true;
    return false;
  }

private:
  auto initialize() -> bool {
    terminate();
    if(!self.context) return false;

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

    DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInputContext, 0);
    if(!directInputContext) return false;

    if(!keyboard.initialize()) return false;
    if(!mouse.initialize(self.context)) return false;
    bool xinputAvailable = joypadXInput.initialize();
    if(!joypadDirectInput.initialize(self.context, directInputContext, xinputAvailable)) return false;
    return isReady = true;
  }

  auto terminate() -> void {
    isReady = false;

    keyboard.terminate();
    mouse.terminate();
    joypadXInput.terminate();
    joypadDirectInput.terminate();

    if(directInputContext) {
      directInputContext->Release();
      directInputContext = nullptr;
    }
  }

  bool isReady = false;
  InputKeyboardRawInput keyboard;
  InputMouseRawInput mouse;
  InputJoypadXInput joypadXInput;
  InputJoypadDirectInput joypadDirectInput;
  LPDIRECTINPUT8 directInputContext = nullptr;
};
