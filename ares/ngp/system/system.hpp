struct System {
  Node::Object node;
  Node::Boolean fastBoot;

  struct Controls {
    Node::Object node;
    Node::Button up;
    Node::Button down;
    Node::Button left;
    Node::Button right;
    Node::Button a;
    Node::Button b;
    Node::Button option;
    Node::Button debugger;
    Node::Button power;

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

  enum class Model : uint { NeoGeoPocket, NeoGeoPocketColor };
  Memory::Readable<uint8> bios;

  auto model() const -> Model { return information.model; }
  auto frequency() const -> double { return 6'144'000; }

  //system.cpp
  auto run() -> void;

  auto load(Node::Object&) -> void;
  auto save() -> void;
  auto unload() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Model model = Model::NeoGeoPocket;
    uint32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern System system;

auto Model::NeoGeoPocket() -> bool { return system.model() == System::Model::NeoGeoPocket; }
auto Model::NeoGeoPocketColor() -> bool { return system.model() == System::Model::NeoGeoPocketColor; }
