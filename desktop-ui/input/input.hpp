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
  auto rumble(u16 strong, u16 weak) -> void;
};

struct InputHotkey : InputDigital {
  InputHotkey(string name) : name(name) {}
  auto& onPress(function<void ()> press) { return this->press = press, *this; }
  auto& onRelease(function<void ()> release) { return this->release = release, *this; }
  auto value() -> s16 override;

  const string name;

private:
  function<void ()> press;
  function<void ()> release;
  s16 state = 0;
  friend struct InputManager;
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
  InputDigital south;
  InputDigital east;
  InputDigital west;
  InputDigital north;
  InputDigital l_bumper;
  InputDigital r_bumper;
  InputAnalog  l_trigger;
  InputAnalog  r_trigger;
  InputDigital lstick_click;
  InputDigital rstick_click;
  InputAnalog  lstick_up;
  InputAnalog  lstick_down;
  InputAnalog  lstick_left;
  InputAnalog  lstick_right;
  InputAnalog  rstick_up;
  InputAnalog  rstick_down;
  InputAnalog  rstick_left;
  InputAnalog  rstick_right;
  InputRumble  rumble;
  InputDigital one;
  InputDigital two;
  InputDigital three;
  InputDigital four;
  InputDigital five;
  InputDigital six;
  InputDigital seven;
  InputDigital eight;
  InputDigital nine;
  InputDigital zero;
  InputDigital star;
  InputDigital clear;
  InputDigital pound;
  InputDigital point;
  InputDigital end;
};

struct VirtualMouse : InputDevice {
  VirtualMouse();

  InputRelative x;
  InputRelative y;
  InputDigital  left;
  InputDigital  middle;
  InputDigital  right;
  InputDigital  extra;
};

struct VirtualKeyboard : InputDevice {
  VirtualKeyboard();

  // The keyboard is represented as a matrix of buttons. Each button
  // corresponds to a key on the keyboard. The matrix is scanned by
  // selecting a column and reading the rows.
  InputDigital esc, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12;
  InputDigital print_screen, scroll_lock, pause;
  InputDigital tilde, num1, num2, num3, num4, num5, num6, num7, num8, num9, num0, dash, equals, backspace;
  InputDigital insert, delete_, home, end, page_up, page_down;
  InputDigital tab, q, w, e, r, t, y, u, i, o, p, lbracket, rbracket, backslash;
  InputDigital capslock, a, s, d, f, g, h, j, k, l, semicolon, apostrophe, return_;
  InputDigital lshift, z, x, c, v, b, n, m, comma, period, slash, rshift;
  InputDigital lctrl, lsuper, lalt, spacebar, ralt, rsuper, rctrl;
  InputDigital up, down, left, right;
  InputDigital numlock, divide, multiply, minus;
  InputDigital keypad7, keypad8, keypad9, add;
  InputDigital keypad4, keypad5, keypad6;
  InputDigital keypad1, keypad2, keypad3, enter;
  InputDigital keypad0, point;
};

struct VirtualPort {
  VirtualPad   pad;
  VirtualMouse mouse;
  VirtualKeyboard keyboard;
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
