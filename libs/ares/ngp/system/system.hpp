struct System {
  Node::System node;
  Node::Setting::Boolean fastBoot;
  VFS::Pak pak;
  Memory::Readable<n8> bios;

  struct Controls {
    Node::Object node;
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button a;
    Node::Input::Button b;
    Node::Input::Button option;
    Node::Input::Button debugger;
    Node::Input::Button power;

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

  struct Debugger {
    System& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory bios;
    } memory;
  } debugger{*this};

  enum class Model : u32 { NeoGeoPocket, NeoGeoPocketColor };

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto frequency() const -> f64 { return 6'144'000; }

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
    string name;
    Model model = Model::NeoGeoPocket;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::NeoGeoPocket() -> bool { return system.model() == System::Model::NeoGeoPocket; }
auto Model::NeoGeoPocketColor() -> bool { return system.model() == System::Model::NeoGeoPocketColor; }
