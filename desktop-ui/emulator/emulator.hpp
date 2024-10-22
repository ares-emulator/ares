struct Emulator {
  //emulators.cpp
  static auto construct() -> void;

  struct Firmware;
  virtual ~Emulator() = default;

  //emulator.cpp
  static auto enumeratePorts(string name) -> vector<InputPort>&;
  auto location() -> string;
  auto locate(const string& location, const string& suffix, const string& path = "", maybe<string> system = {}) -> string;
  auto region() -> string;
  auto load(const string& location) -> bool;
  auto load(shared_pointer<mia::Pak> pak, string& path) -> string;
  auto loadFirmware(const Firmware&) -> shared_pointer<vfs::file>;
  virtual auto unload() -> void;
  auto refresh() -> void;
  auto setBoolean(const string& name, bool value) -> bool;
  auto setOverscan(bool value) -> bool;
  auto setColorBleed(bool value) -> bool;
  auto error(const string& text) -> void;
  auto errorFirmware(const Firmware&, string system = "") -> void;
  auto load(mia::Pak& node, string name) -> bool;
  auto save(mia::Pak& node, string name) -> bool;
  virtual auto input(ares::Node::Input::Input) -> void;
  auto inputKeyboard(string name) -> bool;
  virtual auto load(Menu) -> void {}
  virtual auto load() -> bool = 0;
  virtual auto save() -> bool { return true; }
  virtual auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> = 0;
  virtual auto notify(const string& message) -> void {}
  virtual auto arcade() -> bool { return false; }
  virtual auto group() -> string { return manufacturer; }

  struct Firmware {
    string type;
    string region;
    string sha256;  //optional
    string location;
  };

  string manufacturer;
  string name;
  string medium;
  ares::Node::System root;
  vector<Firmware> firmware;
  shared_pointer<mia::Pak> system;
  shared_pointer<mia::Pak> game;
  shared_pointer<mia::Pak> gamepad;
  shared_pointer<mia::Pak> gb;
  vector<InputPort> ports;
  vector<string> inputBlacklist;
  vector<string> portBlacklist;

  struct Configuration {
    bool visible = true;  //whether or not to show this emulator in the load menu
    string game;          //the most recently used folder for games for each emulator core
  } configuration;

  struct Latch {
    u32 width = 0;
    u32 height = 0;
    u32 rotation = 0;
    bool changed = false;  //used to signal Program::main() to resize the presentation window
  } latch;

  //queue of pre-specified game locations; used by Program::load()
  vector<string> locationQueue;
};

extern vector<shared_pointer<Emulator>> emulators;
extern shared_pointer<Emulator> emulator;
