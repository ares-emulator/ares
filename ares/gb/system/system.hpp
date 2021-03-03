struct System {
  Node::System node;
  Node::Setting::Boolean fastBoot;
  VFS::Pak pak;

  struct Controls {
    Node::Object node;
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button b;
    Node::Input::Button a;
    Node::Input::Button select;
    Node::Input::Button start;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;

    bool yHold = 0;
    bool upLatch = 0;
    bool downLatch = 0;
    bool xHold = 0;
    bool leftLatch = 0;
    bool rightLatch = 0;
  } controls;

  enum class Model : u32 {
    GameBoy,
    GameBoyColor,
    SuperGameBoy,
  };
  Memory::Readable<n8> bootROM;

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto clocksExecuted() const -> u32 { return information.clocksExecuted; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;
  auto clocksExecuted() -> u32;

  auto load(Node::System& node, string name) -> bool;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset = false) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  struct Information {
    string name = "Game Boy";
    Model model = Model::GameBoy;
    n32 clocksExecuted;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;
extern SuperGameBoyInterface* superGameBoy;

auto Model::GameBoy() -> bool { return system.model() == System::Model::GameBoy; }
auto Model::GameBoyColor() -> bool { return system.model() == System::Model::GameBoyColor; }
auto Model::SuperGameBoy() -> bool { return system.model() == System::Model::SuperGameBoy; }
