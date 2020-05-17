//Reality Display Processor

struct RDP : Thread {
  Node::Component node;

  //rdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //render.cpp
  auto render() -> void;
  auto noOperation() -> void;
  auto invalidOperation() -> void;
  auto syncLoad() -> void;
  auto syncPipe() -> void;
  auto syncTile() -> void;
  auto syncFull() -> void;
  auto textureRectangle(
    uint12 xl, uint12 yl, uint3 tile, uint12 xh, uint12 yh,
    uint16 s, uint16 t, uint16 dsdx, uint16 dsdy
  ) -> void;
  auto textureRectangleFlip(
    uint12 xl, uint12 yl, uint3 tile, uint12 xh, uint12 yh,
    uint16 s, uint16 t, uint16 dsdx, uint16 dsdy
  ) -> void;
  auto fillRectangle(uint12 xl, uint12 yl, uint12 xh, uint12 yh) -> void;
  auto setFillColor(uint32 color) -> void;
  auto setTextureImage(uint3 format, uint2 size, uint10 width, uint26 dramAddress) -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, uint32 data) -> void;

  //io-scc.cpp
  auto readSCC(u32 address) -> u32;
  auto writeSCC(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Command {
     uint1 source = 0;  //0 = RDRAM, 1 = DMEM
    uint24 start = 0;
    uint24 end = 0;
    uint24 current = 0;
    uint24 clock = 0;
    uint24 bufferBusy = 0;
    uint24 pipeBusy = 0;
    uint24 tmemBusy = 0;
  } command;

  struct Set {
    struct Fill {
      uint32 color = 0;
    } fill;
    struct Texture {
      uint3 format = 0;
      uint2 size = 0;
      uint10 width = 0;
      uint26 dramAddress = 0;
    } texture;
  } set;

  struct IO {
    struct BIST {
      uint1 check = 0;
      uint1 go = 0;
      uint1 done = 0;
      uint8 fail = 0x00;
    } bist;
    struct Test {
       uint1 enable = 0;
       uint7 address = 0;
      uint32 data = 0;
    } test;
  } io;
};

extern RDP rdp;
