//NEC VR4300

struct CPU : VR4300, Thread {
  Node::Component node;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern CPU cpu;
