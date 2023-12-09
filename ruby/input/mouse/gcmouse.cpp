#pragma once

struct InputMouseGC {
  Input& input;
  InputMouseGC(Input& input) : input(input) {}

  uintptr handle = 0;
  bool mouseAcquired = false;

  struct Mouse {
  } ms;

  auto acquired() -> bool {
    return false;
  }

  auto acquire() -> bool {
    return false;
  }

  auto release() -> bool {
    return true;
  }
    
  auto poll(vector<shared_pointer<HID::Device>>& devices) -> void {}

  auto initialize(uintptr handle) -> bool {
    return true;
  }

  auto terminate() -> void {}
};
