struct System {
  Node::Object node;

  struct Controls {
    Node::Object node;
    Node::Button up;
    Node::Button down;
    Node::Button left;
    Node::Button right;
    Node::Button b;
    Node::Button a;
    Node::Button select;
    Node::Button start;

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

  auto model() const -> Model { return information.model; }
  auto clocksExecuted() const -> uint { return information.clocksExecuted; }

  //system.cpp
  auto run() -> void;
  auto clocksExecuted() -> uint;

  auto load(Node::Object&) -> void;
  auto unload() -> void;
  auto save() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  struct Information {
    Model model = Model::GameBoy;
    uint32 serializeSize[2];
    uint32 clocksExecuted;
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

#include <gb/interface/interface.hpp>

extern System system;

auto Model::GameBoy() -> bool { return system.model() == System::Model::GameBoy; }
auto Model::GameBoyColor() -> bool { return system.model() == System::Model::GameBoyColor; }
auto Model::SuperGameBoy() -> bool { return system.model() == System::Model::SuperGameBoy; }
