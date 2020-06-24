//Sound Processing Unit

struct SPU : Thread {
  Node::Component node;
  Node::Stream stream;

  //spu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto sample() -> void;
  auto step(uint clocks) -> void;

  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    //SPUCNT
    uint1 cdAudioEnable = 0;
    uint1 externalAudioEnable = 0;
    uint1 cdAudioReverb = 0;
    uint1 externalAudioReverb = 0;
    uint2 ramTransferMode = 0;

    //SPURAMCNT
    uint3 ramTransferType = 0;
  } io;
};

extern SPU spu;
