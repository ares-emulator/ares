struct PSG : T6W28, Thread {
  Node::Component node;
  Node::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto enablePSG() -> void;
  auto enableDAC() -> void;
  auto writeLeftDAC(uint8 data) -> void;
  auto writeRightDAC(uint8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct {
    uint1 enable;
  } psg;

  struct DAC {
    uint8 left;
    uint8 right;
  } dac;

private:
  double volume[16];
};

extern PSG psg;
