struct PSG : T6W28, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

auto writePitch(u32) -> void override;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto enablePSG() -> void;
  auto enableDAC() -> void;
  auto writeLeftDAC(n8 data) -> void;
  auto writeRightDAC(n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct {
    n1 enable;
  } psg;

  struct DAC {
    n8 left;
    n8 right;
  } dac;

private:
  f64 volume[16];
};

extern PSG psg;
