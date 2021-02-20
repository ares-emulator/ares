enum : u32 { BindingLimit = 3 };

struct InputMapping {
  enum class Qualifier : u32 { None, Lo, Hi, Rumble };

  InputMapping(const string& name) : name(name) {}

  auto bind() -> void;
  auto bind(u32 binding, string assignment) -> void;
  auto unbind() -> void;
  auto unbind(u32 binding) -> void;

  virtual auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool = 0;
  virtual auto value() -> s16 = 0;

  const string name;
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
  using InputMapping::InputMapping;
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

struct InputAxis : InputMapping {
  using InputMapping::InputMapping;
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
};

struct InputRumble : InputMapping {
  using InputMapping::InputMapping;
  using InputMapping::bind;
  auto bind(u32 binding, shared_pointer<HID::Device>, u32 groupID, u32 inputID, s16 oldValue, s16 newValue) -> bool override;
  auto value() -> s16 override;
  auto rumble(bool enable) -> void;
};

struct InputHotkey : InputButton {
  using InputButton::InputButton;
  auto& onPress(function<void ()> press) { return this->press = press, *this; }
  auto& onRelease(function<void ()> release) { return this->release = release, *this; }

private:
  function<void ()> press;
  function<void ()> release;
  s16 state = 0;
  friend class InputManager;
};

struct VirtualPad {
  VirtualPad();

  InputButton up{"Up"};
  InputButton down{"Down"};
  InputButton left{"Left"};
  InputButton right{"Right"};
  InputButton select{"Select"};
  InputButton start{"Start"};
  InputButton a{"A"};
  InputButton b{"B"};
  InputButton c{"C"};
  InputButton x{"X"};
  InputButton y{"Y"};
  InputButton z{"Z"};
  InputButton l1{"L1"};
  InputButton r1{"R1"};
  InputButton l2{"L2"};
  InputButton r2{"R2"};
  InputButton lt{"LT"};
  InputButton rt{"RT"};
  InputAxis   lx{"LX"};
  InputAxis   ly{"LY"};
  InputAxis   rx{"RX"};
  InputAxis   ry{"RY"};
  InputRumble rumble{"Rumble"};

  vector<InputMapping*> mappings;
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
