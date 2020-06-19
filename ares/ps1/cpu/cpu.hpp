//LSI CoreWare CW33300 (MIPS R3000A)

struct CPU : Thread {
  Node::Component node;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  #include "core/core.hpp"
};

extern CPU cpu;
