struct PSG : SN76489, Thread {
  Node::Component node;
  Node::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto balance(uint8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IO {
    uint8 enable = 0xff;
  };

  IO io;
  double volume[16];
};

extern PSG psg;
