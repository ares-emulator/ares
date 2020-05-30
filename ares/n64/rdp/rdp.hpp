//Reality Display Processor

struct RDP : Thread, Memory::IO<RDP> {
  Node::Component node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto command(string_view) -> void;
    auto io(string_view) -> void;

    struct Tracer {
      Node::Notification command;
      Node::Notification io;
    } tracer;
  } debugger;

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
  auto unshadedTriangle() -> void;
  auto unshadedZbufferTriangle() -> void;
  auto textureTriangle() -> void;
  auto textureZbufferTriangle() -> void;
  auto shadedTriangle() -> void;
  auto shadedZbufferTriangle() -> void;
  auto shadedTextureTriangle() -> void;
  auto shadedTextureZbufferTriangle() -> void;
  auto syncLoad() -> void;
  auto syncPipe() -> void;
  auto syncTile() -> void;
  auto syncFull() -> void;
  auto setKeyGB() -> void;
  auto setKeyR() -> void;
  auto setConvert() -> void;
  auto setScissor() -> void;
  auto setPrimitiveDepth() -> void;
  auto setOtherModes() -> void;
  auto textureRectangle() -> void;
  auto textureRectangleFlip() -> void;
  auto loadTLUT() -> void;
  auto setTileSize() -> void;
  auto loadBlock() -> void;
  auto loadTile() -> void;
  auto setTile() -> void;
  auto fillRectangle() -> void;
  auto setFillColor() -> void;
  auto setFogColor() -> void;
  auto setBlendColor() -> void;
  auto setPrimitiveColor() -> void;
  auto setEnvironmentColor() -> void;
  auto setCombineMode() -> void;
  auto setTextureImage() -> void;
  auto setMaskImage() -> void;
  auto setColorImage() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Command {
    uint24 start = 0;
    uint24 end = 0;
    uint24 current = 0;
    uint24 clock = 0;
    uint24 bufferBusy = 0;
    uint24 pipeBusy = 0;
    uint24 tmemBusy = 0;
     uint1 source = 0;  //0 = RDRAM, 1 = DMEM
     uint1 freeze = 0;
     uint1 flush = 0;
     uint1 ready = 1;
  } command;

  struct Point {
    uint16 i;  //integer
    uint16 f;  //fraction
  };

  struct Edge {
     uint1 lmajor;
     uint3 level;
     uint3 tile;
    struct Y {
      uint14 hi;
      uint14 md;
      uint14 lo;
    } y;
    struct X {
      struct {
        Point c;  //coordinate
        Point s;  //inverse slope
      } hi, md, lo;
    } x;
  } edge;

  struct Shade {
    struct Channel {
      Point c;  //color
      Point x;  //change per X coordinate
      Point y;  //change per Y coordinate
      Point e;  //change along edge
    } r, g, b, a;
  } shade;

  struct Texture {
    struct {
      Point c;  //coordinate
      Point x;  //change per X coordinate
      Point y;  //change per Y coordinate
      Point e;  //change along edge
    } s, t, w;
  } texture;

  struct Zbuffer {
    Point d;  //inverse depth
    Point x;  //change per X coordinate
    Point y;  //change per Y coordinate
    Point e;  //change along edge
  } zbuffer;

  struct TextureRectangle {
    uint3 tile;
    struct {
      uint12 hi;
      uint12 lo;
    } x, y;
    Point s;
    Point t;
  } rectangle;

  struct OtherModes {
    uint1 atomicPrimitive;
    uint1 reserved1;
    uint2 cycleType;
    uint1 perspective;
    uint1 detailTexture;
    uint1 sharpenTexture;
    uint1 lodTexture;
    uint1 tlut;
    uint1 tlutType;
    uint1 sampleType;
    uint1 midTexel;
    uint1 bilerp[2];
    uint1 convertOne;
    uint1 colorKey;
    uint2 colorDitherMode;
    uint2 alphaDitherMode;
    uint4 reserved2;
    uint2 blend1a[2];
    uint2 blend1b[2];
    uint2 blend2a[2];
    uint2 blend2b[2];
    uint1 reserved3;
    uint1 forceBlend;
    uint1 alphaCoverage;
    uint1 coverageXalpha;
    uint2 zMode;
    uint2 coverageMode;
    uint1 colorOnCoverage;
    uint1 imageRead;
    uint1 zUpdate;
    uint1 zCompare;
    uint1 antialias;
    uint1 zSource;
    uint1 ditherAlpha;
    uint1 alphaCompare;
  } other;

  struct FogColor {
    uint8 red;
    uint8 green;
    uint8 blue;
    uint8 alpha;
  } fog;

  struct Blend {
    uint8 red;
    uint8 green;
    uint8 blue;
    uint8 alpha;
  } blend;

  struct PrimitiveColor {
    uint5 minimum;
    uint8 fraction;
    uint8 red;
    uint8 green;
    uint8 blue;
    uint8 alpha;
  } primitive;

  struct EnvironmentColor {
    uint8 red;
    uint8 green;
    uint8 blue;
    uint8 alpha;
  } environment;

  struct CombineMode {
    struct MUL {
      uint5 color[2];
      uint3 alpha[2];
    } mul;
    struct ADD {
      uint3 color[2];
      uint3 alpha[2];
    } add;
    struct SUB {
      uint4 color[2];
      uint3 alpha[2];
    } sba, sbb;
  } combine;

  struct TLUT {
    uint3 index;
    struct {
      uint12 lo;
      uint12 hi;
    } s, t;
  } tlut;

  struct Load {
    struct Block {
      uint3 index;
      struct {
        uint12 lo;
        uint12 hi;
      } s, t;
    } block;
    struct Tile {
      uint3 index;
      struct {
        uint12 lo;
        uint12 hi;
      } s, t;
    } tile;
  } load_;

  struct TileSize {
    uint3 index;
    struct {
      uint12 lo;
      uint12 hi;
    } s, t;
  } tileSize;

  struct Tile {
    uint3 format;
    uint2 size;
    uint9 line;
    uint9 address;
    uint3 index;
    uint4 palette;
    struct {
      uint1 clamp;
      uint1 mirror;
      uint4 mask;
      uint4 shift;
    } s, t;
  } tile;

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
    struct Mask {
      uint26 dramAddress = 0;
    } mask;
    struct Color {
       uint3 format = 0;
       uint2 size = 0;
      uint10 width = 0;
      uint26 dramAddress = 0;
    } color;
  } set;

  struct PrimitiveDepth {
    uint16 z;
    uint16 deltaZ;
  } primitiveDepth;

  struct Scissor {
    uint1 field;
    uint1 odd;
    struct {
      uint12 lo;
      uint12 hi;
    } x, y;
  } scissor;

  struct Convert {
    uint9 k[6];
  } convert;

  struct Key {
    struct {
      uint12 width;
       uint8 center;
       uint8 scale;
    } r, g, b;
  } key;

  struct FillRectangle {
    struct {
      uint12 lo;
      uint12 hi;
    } x, y;
  } fillRectangle_;

  struct IO : Memory::IO<IO> {
    RDP& self;
    IO(RDP& self) : self(self) {}

    //io.cpp
    auto readWord(u32 address) -> u32;
    auto writeWord(u32 address, u32 data) -> void;

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
  } io{*this};
};

extern RDP rdp;
