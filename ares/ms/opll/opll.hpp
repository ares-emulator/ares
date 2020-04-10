struct OPLL : YM2413, Thread {
  Node::Component node;
  Node::Stream stream;

  //opll.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern OPLL opll;
