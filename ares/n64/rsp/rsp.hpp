//Reality Signal Processor

struct RSP : Thread {
  Node::Component node;
  Node::Stream stream;
  Memory dmem;
  Memory imem;

  //rsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern RSP rsp;
