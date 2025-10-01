#include "keyboard/carbon.cpp"

struct InputCarbon : InputDriver {
  InputCarbon& self = *this;
  InputCarbon(Input& super) : InputDriver(super), keyboard(super) {}
  ~InputCarbon() { terminate(); }

  auto create() -> bool override {
    return initialize();
  }

  auto driver() -> string override { return "Carbon"; }
  auto ready() -> bool override { return isReady; }

  auto acquired() -> bool override { return false; }
  auto acquire() -> bool override { return false; }
  auto release() -> bool override { return false; }

  auto poll() -> std::vector<std::shared_ptr<HID::Device>> override {
    std::vector<std::shared_ptr<HID::Device>> devices;
    keyboard.poll(devices);
    return devices;
  }

  auto rumble(u64 id, u16 strong, u16 weak) -> bool override {
    return false;
  }

private:
  auto initialize() -> bool {
    terminate();
    if(!keyboard.initialize()) return false;
    return isReady = true;
  }

  auto terminate() -> void {
    isReady = false;
    keyboard.terminate();
  }

  bool isReady = false;
  InputKeyboardCarbon keyboard;
};
