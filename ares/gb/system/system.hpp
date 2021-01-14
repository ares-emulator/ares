struct System {
  Node::System node;
  Node::Setting::Boolean fastBoot;

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

  enum class Model : uint {
    GameBoy,
    GameBoyColor,
    SuperGameBoy,
  };
  Memory::Readable<uint8> bootROM;

  auto name() const -> string { return node->name(); }
  auto model() const -> Model { return information.model; }
  auto clocksExecuted() const -> uint { return information.clocksExecuted; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;
  auto clocksExecuted() -> uint;

  auto load(Node::System& node, string name) -> bool;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset = false) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  struct Information {
    Model model = Model::GameBoy;
    uint32 clocksExecuted;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;
extern SuperGameBoyInterface* superGameBoy;

auto Model::GameBoy() -> bool { return system.model() == System::Model::GameBoy; }
auto Model::GameBoyColor() -> bool { return system.model() == System::Model::GameBoyColor; }
auto Model::SuperGameBoy() -> bool { return system.model() == System::Model::SuperGameBoy; }
