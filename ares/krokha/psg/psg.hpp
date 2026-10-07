//a bare piezo speaker driven from one bit of the latch at 0xf7ff; there is no
//sound chip to emulate, only the level the CPU last wrote

struct PSG : Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto write(n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  b1 level = 0;
};

extern PSG psg;
