struct Emulator {
  struct Firmware;
  static auto construct() -> void;
  virtual ~Emulator() = default;
  auto locate(const string& location, const string& suffix, const string& path = "") -> string;
  auto manifest() -> shared_pointer<vfs::file>;
  auto manifest(const string& location) -> shared_pointer<vfs::file>;
  auto manifest(const string& type, const string& location) -> shared_pointer<vfs::file>;
  auto region() -> string;
  auto load(const string& location, const vector<u8>& image) -> bool;
  auto loadFirmware(const Firmware&) -> shared_pointer<vfs::file>;
  auto save() -> void;
  auto unload() -> void;
  auto refresh() -> void;
  auto setBoolean(const string& name, bool value) -> bool;
  auto setOverscan(bool value) -> bool;
  auto error(const string& text) -> void;
  auto errorFirmwareRequired(const Firmware&) -> void;
  virtual auto load(Menu) -> void {}
  virtual auto load() -> bool = 0;
  virtual auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> = 0;
  virtual auto input(ares::Node::Input::Input) -> void = 0;
  virtual auto notify(const string& message) -> void {}

  struct Firmware {
    string type;
    string region;
    string sha256;  //optional
    string location;
  };

  struct Game {
    string location;
    string manifest;
    vector<u8> image;
  };

  shared_pointer<mia::Media> medium;
  string manufacturer;
  string name;

  ares::Node::System root;
  vector<Firmware> firmware;
  Game game;

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
};

extern vector<shared_pointer<Emulator>> emulators;
extern shared_pointer<Emulator> emulator;
