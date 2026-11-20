//Reality Display Processor
#include <n64/rdp/capture.hpp>

struct RDP : Thread, Memory::RCP<RDP> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto command(string_view) -> void;
    auto ioDPC(bool mode, u32 address, u32 data) -> void;
    auto ioDPS(bool mode, u32 address, u32 data) -> void;
    auto updateGraphicsSize() -> void;
    auto observeCommand(u32 code, const u32* words, u32 wordCount) -> void;

    //capture.cpp
    auto requestCapture() -> void;
    auto captureActive() const -> bool;
    auto captureReady() const -> bool;
    auto frameBoundary() -> void;
    auto presentBoundary(u32 origin) -> void;
    auto beginCapture() -> void;
    auto endCapture() -> void;
    auto commandBatch() -> void;
    auto captureCommand(const u32* words, u32 wordCount) -> void;
    auto captureViRegister(u32 address, u32 value) -> void;
    auto captureScanout(bool field) -> void;
    auto resetCapture() -> void;
    auto frameCapture() const -> const RDPFrameCapture*;
    auto captureCommandCount() const -> u32;
    auto capturedCommandWords(u32 index) const -> const std::vector<u32>*;
    auto captureCommandText(u32 index) const -> string;
    auto captureCommandArguments(u32 index) const -> string;
    auto captureCommandDetail(u32 index) const -> string;
    auto captureCommandOpcode(u32 index) const -> u32;
    auto captureCommandType(u32 index) const -> u32;
    auto captureSummary() const -> string;
    auto captureStateAt(u32 command) const -> RDPCaptureState;
    auto captureStateText(u32 command) const -> string;
    auto captureViews() const -> std::vector<string>;
    auto captureTicks() -> std::vector<u32>;
    auto renderCapture(u32 command, u32 view) -> Core::Debugger::GraphicsFrame::Image;
    auto renderCaptureTile(
      const u8* tmem, const RDPCaptureState& state, u32 tile
    ) const -> Core::Debugger::GraphicsFrame::Image;

    struct Capture {
      std::atomic<bool> armed = false;
      std::atomic<bool> ready = false;
      bool active = false;
      //Set once a command list has been diffed; cleared by Sync Full. RDRAM is
      //only sampled between command lists, where the RDP is idle.
      bool inCommandList = false;
      //Fields seen while armed and while capturing, counted only to drive the
      //fallback for content that never presents a different framebuffer.
      u32 armedFields = 0;
      u32 activeFields = 0;
      RDPFrameCapture frame;
      std::vector<u8> shadowRdram;
    };
    std::unique_ptr<Capture> capture;
    u64 nextCaptureIdentifier = 1;
    //Framebuffer the VI is presenting, in 4 KiB units; changes mark a buffer flip.
    u32 presentBuffer = ~0u;

    //captureTicks() replays the whole capture, so its result is cached against
    //the capture it was computed from.
    u64 tickIdentifier = 0;
    std::vector<u32> tickCommands;

    struct CachedCommand {
      u64 sequence = 0;
      std::vector<u32> words;
    };
    std::vector<CachedCommand> cachedCommands;
    u64 cachedCommandSequence = 0;

    struct Graphics {
      Node::Debugger::Graphics color;
      Node::Debugger::Graphics depth;
      Node::Debugger::Graphics coverage;
    } graphics;

    Node::Debugger::Memory tmem;
    Node::Debugger::GraphicsFrame frame;

    struct Tracer {
      Node::Debugger::Tracer::Notification command;
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //rdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power(bool reset) -> void;
  auto crash(const char *reason) -> void;

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
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;
  auto flushCommands() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Command {
    n24 start;
    n24 end;
    n24 current;
    n24 clock;
    n24 bufferBusy;
    n24 pipeBusy;
    n24 tmemBusy;
    n1  source;  //0 = RDRAM, 1 = DMEM
    n1  freeze;
    n1  crashed;
    n1  flush;
    n1  startValid;
    n1  endValid;
    n1  startGclk;
    n1  ready = 1;
  } command;

  struct Point {
    n16 i;  //integer
    n16 f;  //fraction
  };

  struct Edge {
    n1 lmajor;
    n3 level;
    n3 tile;
    struct Y {
      n14 hi;
      n14 md;
      n14 lo;
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
    n3 tile;
    struct {
      n12 hi;
      n12 lo;
    } x, y;
    Point s;
    Point t;
  } rectangle;

  struct OtherModes {
    n1 atomicPrimitive;
    n1 reserved1;
    n2 cycleType;
    n1 perspective;
    n1 detailTexture;
    n1 sharpenTexture;
    n1 lodTexture;
    n1 tlut;
    n1 tlutType;
    n1 sampleType;
    n1 midTexel;
    n1 bilerp[2];
    n1 convertOne;
    n1 colorKey;
    n2 colorDitherMode;
    n2 alphaDitherMode;
    n4 reserved2;
    n2 blend1a[2];
    n2 blend1b[2];
    n2 blend2a[2];
    n2 blend2b[2];
    n1 reserved3;
    n1 forceBlend;
    n1 alphaCoverage;
    n1 coverageXalpha;
    n2 zMode;
    n2 coverageMode;
    n1 colorOnCoverage;
    n1 imageRead;
    n1 zUpdate;
    n1 zCompare;
    n1 antialias;
    n1 zSource;
    n1 ditherAlpha;
    n1 alphaCompare;
  } other;

  struct FogColor {
    n8 red;
    n8 green;
    n8 blue;
    n8 alpha;
  } fog;

  struct Blend {
    n8 red;
    n8 green;
    n8 blue;
    n8 alpha;
  } blend;

  struct PrimitiveColor {
    n5 minimum;
    n8 fraction;
    n8 red;
    n8 green;
    n8 blue;
    n8 alpha;
  } primitive;

  struct EnvironmentColor {
    n8 red;
    n8 green;
    n8 blue;
    n8 alpha;
  } environment;

  struct CombineMode {
    struct MUL {
      n5 color[2];
      n3 alpha[2];
    } mul;
    struct ADD {
      n3 color[2];
      n3 alpha[2];
    } add;
    struct SUB {
      n4 color[2];
      n3 alpha[2];
    } sba, sbb;
  } combine;

  struct TLUT {
    n3 index;
    struct {
      n12 lo;
      n12 hi;
    } s, t;
  } tlut;

  struct Load {
    struct Block {
      n3 index;
      struct {
        n12 lo;
        n12 hi;
      } s, t;
    } block;
    struct Tile {
      n3 index;
      struct {
        n12 lo;
        n12 hi;
      } s, t;
    } tile;
  } load_;

  struct TileSize {
    n3 index;
    struct {
      n12 lo;
      n12 hi;
    } s, t;
  } tileSize;

  struct Tile {
    n3 format;
    n2 size;
    n9 line;
    n9 address;
    n3 index;
    n4 palette;
    struct {
      n1 clamp;
      n1 mirror;
      n4 mask;
      n4 shift;
    } s, t;
  } tile;

  struct Set {
    struct Fill {
      n32 color = 0;
    } fill;
    struct Texture {
      n3  format = 0;
      n2  size = 0;
      n10 width = 0;
      n26 dramAddress = 0;
    } texture;
    struct Mask {
      n26 dramAddress = 0;
    } mask;
    struct Color {
      n3  format = 0;
      n2  size = 0;
      n10 width = 0;
      n26 dramAddress = 0;
    } color;
  } set;

  struct PrimitiveDepth {
    n16 z;
    n16 deltaZ;
  } primitiveDepth;

  struct Scissor {
    n1 field;
    n1 odd;
    struct {
      n12 lo;
      n12 hi;
    } x, y;
  } scissor;

  struct Convert {
    n9 k[6];
  } convert;

  struct Key {
    struct {
      n12 width;
      n8  center;
      n8  scale;
    } r, g, b;
  } key;

  struct FillRectangle {
    struct {
      n12 lo;
      n12 hi;
    } x, y;
  } fillRectangle_;

  struct IO : Memory::RCP<IO> {
    RDP& self;
    IO(RDP& self) : self(self) {}

    //io.cpp
    auto readWord(u32 address, Thread& thread) -> u32;
    auto writeWord(u32 address, u32 data, Thread& thread) -> void;

    struct BIST {
      n1 check;
      n1 go;
      n1 done;
      n8 fail;
    } bist;
    struct Test {
      n1  enable;
      n7  address;
      array<u32[128]> data;
    } test;

  } io{*this};

  n1 mapIdentityWarned;
};

extern RDP rdp;
