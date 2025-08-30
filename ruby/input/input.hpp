struct Input;

struct InputDriver {
  InputDriver(Input& super) : super(super) {}
  virtual ~InputDriver() = default;

  virtual auto create() -> bool { return true; }
  virtual auto driver() -> string { return "None"; }
  virtual auto ready() -> bool { return true; }

  virtual auto hasContext() -> bool { return false; }

  virtual auto setContext(uintptr context) -> bool { return true; }

  virtual auto acquired() -> bool { return false; }
  virtual auto acquire() -> bool { return false; }
  virtual auto release() -> bool { return false; }
  virtual auto poll() -> std::vector<shared_pointer<nall::HID::Device>> { return {}; }
  virtual auto rumble(u64 id, u16 strong, u16 weak) -> bool { return false; }

protected:
  Input& super;
  friend struct Input;

  uintptr context = 0;
};

struct Input {
  static auto hasDrivers() -> std::vector<string>;
  static auto hasDriver(string driver) -> bool { auto v = hasDrivers(); return std::ranges::find(v, driver) != v.end(); }
  static auto optimalDriver() -> string;
  static auto safestDriver() -> string;

  Input() : self(*this) { reset(); }
  explicit operator bool() { return instance->driver() != "None"; }
  auto reset() -> void { instance = new InputDriver(*this); }
  auto create(string driver = "") -> bool;
  auto driver() -> string { return instance->driver(); }
  auto ready() -> bool { return instance->ready(); }

  auto hasContext() -> bool { return instance->hasContext(); }

  auto context() -> uintptr { return instance->context; }

  auto setContext(uintptr context) -> bool;

  auto acquired() -> bool;
  auto acquire() -> bool;
  auto release() -> bool;
  auto poll() -> std::vector<shared_pointer<nall::HID::Device>>;
  auto rumble(u64 id, u16 strong, u16 weak) -> bool;

  auto onChange(const function<void (shared_pointer<nall::HID::Device>, u32, u32, s16, s16)>&) -> void;
  auto doChange(shared_pointer<nall::HID::Device> device, u32 group, u32 input, s16 oldValue, s16 newValue) -> void;

protected:
  Input& self;
  unique_pointer<InputDriver> instance;
  function<void (shared_pointer<nall::HID::Device> device, u32 group, u32 input, s16 oldValue, s16 newValue)> change;
};
