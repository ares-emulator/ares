struct OPLL : YM2413, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //opll.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n1 mute = 1;
  } io;
};

extern OPLL opll;
