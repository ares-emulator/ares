struct Emulator {
  //emulators.cpp
  static auto construct() -> void;

  struct Firmware;
  virtual ~Emulator() = default;

  //emulator.cpp
  static auto enumeratePorts(string name) -> std::vector<InputPort>&;
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
  auto load(mia::Pak& node, string name) -> bool;
  auto save(mia::Pak& node, string name) -> bool;
  virtual auto input(ares::Node::Input::Input) -> void;
  auto inputKeyboard(string name) -> bool;
  auto handleLoadResult(LoadResult result) -> void;
  virtual auto load(Menu) -> void {}
  virtual auto load() -> LoadResult = 0;
  virtual auto save() -> bool { return true; }
  virtual auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> = 0;
  virtual auto notify(const string& message) -> void {}
  virtual auto arcade() -> bool { return false; }
  virtual auto group() -> string { return manufacturer; }
  virtual auto portMenu(Menu& portMenu, ares::Node::Port port) -> void {}

  virtual auto loadTape(ares::Node::Object node, string location) -> bool { return false; }
  virtual auto unloadTape(ares::Node::Object node) -> void {}

  virtual auto loadSetting(Markup::Node *node) -> void {}
  virtual auto saveSetting(Markup::Node *node) -> void {}
  virtual auto bindInput() -> void {}

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
  std::vector<Firmware> firmware;
  shared_pointer<mia::Pak> system;
  shared_pointer<mia::Pak> game;
  shared_pointer<mia::Pak> gamepad;
  shared_pointer<mia::Pak> gb;
  std::vector<InputPort> ports;
  std::vector<string> inputBlacklist;
  std::vector<string> portBlacklist;

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
  std::vector<string> locationQueue;
};

extern std::vector<shared_pointer<Emulator>> emulators;
extern shared_pointer<Emulator> emulator;
