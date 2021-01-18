struct PSG : SN76489, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto balance(n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n1 mute = 0;
    n8 enable = 0xff;
  };

  IO io;
  f64 volume[16];
};

extern PSG psg;
