//Yamaha YM2612

struct OPN2 : YM2612, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //opn2.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern OPN2 opn2;
