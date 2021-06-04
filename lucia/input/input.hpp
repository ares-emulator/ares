enum : u32 { BindingLimit = 3 };

struct InputMapping {
  enum class Qualifier : u32 { None, Lo, Hi, Rumble };

  auto bind() -> void;
  auto bind(u32 binding, string assignment) -> void;
  auto unbind() -> void;
  auto unbind(u32 binding) -> void;

  virtual auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool = 0;
  virtual auto value() -> s16 = 0;

  string assignments[BindingLimit];

  struct Binding {
    auto icon() -> image;
    auto text() -> string;

    shared_pointer<HID::Device> device;
    u64 deviceID;
    u32 groupID;
    u32 inputID;
    Qualifier qualifier = Qualifier::None;
  };
  Binding bindings[BindingLimit];
};

struct InputButton : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

struct InputAnalog : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

struct InputAxis : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

struct InputRumble : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
  auto rumble(bool enable) -> void;
};

struct InputHotkey : InputButton {
  InputHotkey(string name) : name(name) {}
  auto& onPress(function<void ()> press) { return this->press = press, *this; }
  auto& onRelease(function<void ()> release) { return this->release = release, *this; }

  const string name;

private:
  function<void ()> press;
  function<void ()> release;
  s16 state = 0;
  friend class InputManager;
};

struct InputNode {
  enum class Type : u32 { Analog, Button, Rumble };
  Type type;
  string name;
  InputMapping* mapping;
};

struct InputDevice {
  auto analog(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Analog, name, &mapping});
  }

  auto button(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Button, name, &mapping});
  }

  auto rumble(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Rumble, name, &mapping});
  }

  string name;
  vector<InputNode> inputs;
};

struct InputPort {
  auto append(InputDevice& device) -> void {
    devices.append(device);
  }

  string name;
  vector<InputDevice> devices;
};

struct VirtualPad : InputDevice {
  VirtualPad();

  InputButton up;
  InputButton down;
  InputButton left;
  InputButton right;
  InputButton select;
  InputButton start;
  InputButton a;
  InputButton b;
  InputButton c;
  InputButton x;
  InputButton y;
  InputButton z;
  InputButton l1;
  InputButton r1;
  InputButton l2;
  InputButton r2;
  InputButton lt;
  InputButton rt;
  InputAnalog lup;
  InputAnalog ldown;
  InputAnalog lleft;
  InputAnalog lright;
  InputAnalog rup;
  InputAnalog rdown;
  InputAnalog rleft;
  InputAnalog rright;
  InputRumble rumble;
};

struct InputManager {
  auto create() -> void;
  auto bind() -> void;
  auto poll(bool force = false) -> void;
  auto eventInput(shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> void;

  //hotkeys.cpp
  auto createHotkeys() -> void;
  auto pollHotkeys() -> void;

  vector<shared_pointer<HID::Device>> devices;
  vector<InputHotkey> hotkeys;

  u64 pollFrequency = 5;
  u64 lastPoll = 0;
};

extern VirtualPad virtualPads[2];
extern InputManager inputManager;
