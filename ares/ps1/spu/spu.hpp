//Sound Processing Unit

struct SPU : Thread {
  Node::Component node;

  //spu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern SPU spu;
