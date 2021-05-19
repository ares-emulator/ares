//Yamaha YM2610

struct OPNB : YM2610, Thread {
  Node::Object node;
  Node::Audio::Stream streamFM;
  Node::Audio::Stream streamSSG;

  //opnb.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  f64 volume[16];
};

extern OPNB opnb;
