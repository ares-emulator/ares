//Reality Signal Processor

struct RSP : Thread {
  Node::Component node;
  Memory dmem;
  Memory imem;

  //rsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  #include "core/core.hpp"
};

extern RSP rsp;
