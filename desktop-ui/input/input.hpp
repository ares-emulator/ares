enum : u32 { BindingLimit = 3 };

struct InputMapping {
  enum class Qualifier : u32 { None, Lo, Hi, Rumble };

  auto bind() -> void;
  auto bind(u32 binding, string assignment) -> void;
  auto unbind() -> void;
  auto unbind(u32 binding) -> void;

  virtual auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool = 0;
  virtual auto value() -> s16 = 0;
  virtual auto pressed() -> bool { return false; }

  string assignments[BindingLimit];

  struct Binding {
    auto icon() -> multiFactorImage;
    auto text() -> string;

    shared_pointer<HID::Device> device;
    u64 deviceID;
    u32 groupID;
    u32 inputID;
    Qualifier qualifier = Qualifier::None;
  };
  Binding bindings[BindingLimit];
};

//0 or 1
struct InputDigital : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
  auto pressed() -> bool override;
};

//0 ... +32767
struct InputAnalog : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
  auto pressed() -> bool override;
};

//-32768 ... +32767
struct InputAbsolute : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

//-32768 ... +32767
struct InputRelative : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

//specifies a target joypad for force feedback
struct InputRumble : InputMapping {
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
  auto rumble(bool enable) -> void;
};

struct InputHotkey : InputDigital {
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
  enum class Type : u32 { Digital, Analog, Absolute, Relative, Rumble };
  Type type;
  string name;
  InputMapping* mapping;
};

struct InputPair {
  enum class Type : u32 { Analog };
  Type type;
  string name;
  InputMapping* mappingLo;
  InputMapping* mappingHi;
};

struct InputDevice {
  auto digital(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Digital, name, &mapping});
  }

  auto analog(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Analog, name, &mapping});
  }

  auto absolute(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Absolute, name, &mapping});
  }

  auto relative(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Relative, name, &mapping});
  }

  auto rumble(string name, InputMapping& mapping) -> void {
    inputs.append({InputNode::Type::Rumble, name, &mapping});
  }

  auto analog(string name, InputMapping& mappingLo, InputMapping& mappingHi) -> void {
    pairs.append({InputPair::Type::Analog, name, &mappingLo, &mappingHi});
  }

  string name;
  vector<InputNode> inputs;
  vector<InputPair> pairs;
};

struct InputPort {
  auto append(const InputDevice& device) -> void {
    devices.append(device);
  }

  string name;
  vector<InputDevice> devices;
};

struct VirtualPad : InputDevice {
  VirtualPad();

  InputDigital up;
  InputDigital down;
  InputDigital left;
  InputDigital right;
  InputDigital select;
  InputDigital start;
  InputDigital a;
  InputDigital b;
  InputDigital c;
  InputDigital x;
  InputDigital y;
  InputDigital z;
  InputDigital l1;
  InputDigital r1;
  InputDigital l2;
  InputDigital r2;
  InputDigital lt;
  InputDigital rt;
  InputAnalog  lup;
  InputAnalog  ldown;
  InputAnalog  lleft;
  InputAnalog  lright;
  InputAnalog  rup;
  InputAnalog  rdown;
  InputAnalog  rleft;
  InputAnalog  rright;
  InputRumble  rumble;
};

struct VirtualMouse : InputDevice {
  VirtualMouse();

  InputRelative x;
  InputRelative y;
  InputDigital  left;
  InputDigital  middle;
  InputDigital  right;
};

struct VirtualPort {
  VirtualPad   pad;
  VirtualMouse mouse;
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

extern VirtualPort virtualPorts[5];
extern InputManager inputManager;
