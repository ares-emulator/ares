//Yamaha YM2610

struct OPNB : YM2610, Thread {
  Node::Object node;
  Node::Audio::Stream streamFM;
  Node::Audio::Stream streamSSG;
  Node::Audio::Stream streamPCMA;

  //opnb.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  auto readPCMA(u32 address) -> u8 override;

  i32 cyclesUntilFmSsg;
  i32 cyclesUntilPcmA;
private:
  f64 volume[32];
};

extern OPNB opnb;
