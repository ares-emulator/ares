#include "bios.hpp"

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
    Node::Button l;
    Node::Button r;
    Node::Button select;
    Node::Button start;
    Node::Rumble rumbler;  //Game Boy Player

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

  enum class Model : uint { GameBoyAdvance, GameBoyPlayer };

  auto model() const -> Model { return information.model; }
  auto frequency() const -> double { return 16 * 1024 * 1024; }

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
    Model model = Model::GameBoyAdvance;
    uint32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern System system;

auto Model::GameBoyAdvance() -> bool { return system.model() == System::Model::GameBoyAdvance; }
auto Model::GameBoyPlayer() -> bool { return system.model() == System::Model::GameBoyPlayer; }
