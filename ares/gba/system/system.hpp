struct BIOS {
  Memory::Readable<n8> rom;

  //bios.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto read(u32 mode, n25 address) -> n32;
  auto write(u32 mode, n25 address, n32 word) -> void;
  auto serialize(serializer&) -> void;

  n32 mdr = 0;
};

struct System {
  Node::System node;
  VFS::Pak pak;

  struct Controls {
    Node::Object node;
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button b;
    Node::Input::Button a;
    Node::Input::Button l;
    Node::Input::Button r;
    Node::Input::Button select;
    Node::Input::Button start;
    Node::Input::Rumble rumbler;  //Game Boy Player

    auto load(Node::Object) -> void;
    auto poll() -> void;
    auto rumble(bool enable) -> void;

    bool yHold = 0;
    bool upLatch = 0;
    bool downLatch = 0;
    bool xHold = 0;
    bool leftLatch = 0;
    bool rightLatch = 0;
  } controls;

  enum class Model : u32 { GameBoyAdvance, GameBoyPlayer };

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto frequency() const -> f64 { return 16 * 1024 * 1024; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset = false) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    string name = "Game Boy Advance";
    Model model = Model::GameBoyAdvance;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern BIOS bios;
extern System system;

auto Model::GameBoyAdvance() -> bool { return system.model() == System::Model::GameBoyAdvance; }
auto Model::GameBoyPlayer() -> bool { return system.model() == System::Model::GameBoyPlayer; }
