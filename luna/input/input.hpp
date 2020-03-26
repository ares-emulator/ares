#include "hotkeys.hpp"

struct InputAxis {
  auto value() -> int16_t;

  const string name;
  shared_pointer<HID::Device> device;
  uint groupID;
  uint inputID;
};

struct InputButton {
  auto value() -> int16_t;

  const string name;
  shared_pointer<HID::Device> device;
  uint groupID;
  uint inputID;
  enum class Qualifier : uint { None, Lo, Hi } qualifier = Qualifier::None;
};

struct InputManager {
  auto bind(maybe<ares::Node::Object> root = {}) -> void;
  auto unbind() -> void;

  auto poll() -> void;
  auto eventInput(shared_pointer<HID::Device>, uint group, uint input, int16_t oldValue, int16_t newValue) -> void;

  ares::Node::Object root;
  vector<shared_pointer<HID::Device>> devices;
  Hotkeys hotkeys;

  uint64_t pollFrequency = 5;
  uint64_t lastPoll = 0;
};

extern InputManager inputManager;
extern Hotkeys& hotkeys;
