//Graphics Processing Unit

struct GPU : Thread, Memory::Interface {
  Node::Object node;
  Node::Video::Screen screen;
  Memory::Writable vram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory vram;
    } memory;

    struct Graphics {
      Node::Debugger::Graphics vram15bpp;
      Node::Debugger::Graphics vram24bpp;
    } graphics;
  } debugger;

  auto vtotal() const -> u32 {
    if(io.interlace) return io.videoMode ? 313 : 263;
    return io.videoMode ? 314 : 263;
  }
  auto halfLine() const -> bool { return io.interlace && io.vcounter + 1 == vtotal(); }
  auto vstart() const -> u32 { return io.displayRangeY1; }
  auto vend() const -> u32 { return io.displayRangeY2; }
  auto hblank() const -> bool { return io.hcounter >= hblankStart(); }
  auto vblank() const -> bool { return io.vcounter < vstart() || io.vcounter >= vend(); }
  auto interlace() const -> bool { return io.verticalResolution && io.interlace; }
  auto htotalNumerator() const -> u64 { return io.videoMode ? 6'812 : 6'825; }
  auto htotalDenominator() const -> u32 { return 2; }
  auto htotal() const -> u32 { return htotalNumerator() / htotalDenominator(); }
  auto hblankStart() const -> u32 { return io.displayRangeX2; }
  auto displayWidth() const -> u32 { return io.horizontalResolution < 4 ? displayWidths[io.horizontalResolution] : displayWidths[4]; }
  auto rawDotclockDivider() const -> u32 { return io.horizontalResolution < 4 ? dotclockDividers[io.horizontalResolution] : dotclockDividers[4]; }
  auto dotclockDivider() const -> u32 { return io.dividerPipeline[2]; }

  //gpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto nextHtotal() -> u32;
  auto frame() -> void;
  auto step(u32 clocks) -> void;
  auto advanceDivider() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto writeFifoEmpty() const -> bool;
  auto dmaRequest() const -> bool;
  auto canReadDMA() -> bool;
  auto canWriteDMA() -> bool;
  auto readDMA() -> u32;
  auto writeDMA(u32 data) -> void;

  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //gp0.cpp
  auto readGP0() -> u32;
  auto writeGP0(u32 data, bool isThread = false, bool isDMA = false) -> void;
  auto executeGP0(u32 data, bool isThread = false) -> void;
  auto commandLength(u32 word) -> u32;
  auto executeLatch() -> void;
  auto stepFifo() -> void;
  auto stepFill(u32 clocks) -> void;

  //gp1.cpp
  auto readGP1() -> u32;
  auto writeGP1(u32 data) -> void;

  struct RenderThread {
    GPU& self;
    RenderThread(GPU& self) : self(self) {}

    //thread.cpp
    auto main(uintptr_t) -> void;
    auto power() -> void;
    auto exit() -> void;

    nall::thread handle;
    nall::queue_spsc<u32[65536]> fifo;
    static constexpr u32 Terminate = 0xff0a4350;
  } thread{*this};

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum class Mode : u32 {
    Normal,
    CopyToVRAM,
    CopyFromVRAM,
  };

  struct Display {
    u32  dotclock;
    u32  width;
    u32  height;
    bool interlace;
    struct Previous {
      s32 x;
      s32 y;
      u32 width;
      u32 height;
    } previous;
  } display;

  struct IO {
    Mode mode = Mode::Normal;

    u32 dividerPipeline[3] = {10, 10, 10};
    u32 dividerClocks = 0;
    n1  field;      //even or odd scanline
    n1  displayField;
    n16 hcounter;   //horizontal counter
    n16 vcounter;   //vertical counter
    u32 htotalRemainder;
    i32 pcounter;
    i32 dotcounter;
    i32 timerDotcounter;
    u32 hblankClock;
    n1  hblankFired;

    //GP0(a0): copy rectangle (CPU to VRAM)
    //GP0(c0): copy rectangle (VRAM to CPU)
    struct Copy {
      n16 x;
      n16 y;
      n16 width;
      n16 height;
    //internal:
      n16 px;
      n16 py;
    } copy;

    //GP0(e1): draw mode
    n4 texturePageBaseX;
    n1 texturePageBaseY;
    n2 semiTransparency;
    n2 textureDepth;
    n1 dithering;
    n1 drawToDisplay;
    n1 texturePageBaseY2;
    n1 texturePageBaseY2Enable;
    n1 textureFlipX;
    n1 textureFlipY;

    //GP0(e2): texture window
    n5 textureWindowMaskX;
    n5 textureWindowMaskY;
    n5 textureWindowOffsetX;
    n5 textureWindowOffsetY;

    //GP0(e3): set drawing area (top left)
    n10 drawingAreaOriginX1;
    n10 drawingAreaOriginY1;

    //GP0(e4): set drawing area (bottom right)
    n10 drawingAreaOriginX2;
    n10 drawingAreaOriginY2;

    //GP0(e5): set drawing offset
    n11 drawingAreaOffsetX;
    n11 drawingAreaOffsetY;

    //GP0(e6): mask bit
    n1 forceMaskBit;
    n1 checkMaskBit;

    //GP1(02): acknowledge interrupt
    n1 interrupt;

    //GP1(03): display disable
    n1 displayDisable = 1;

    //GP1(04): DMA direction
    n2 dmaDirection;

    //GP1(05): start of display area
    n10 displayStartX;
    n9  displayStartY;

    //GP1(06): horizontal display range
    n12 displayRangeX1 = 512;
    n12 displayRangeX2 = 512 + 256 * 10;

    //GP1(07): vertical display range
    n12 displayRangeY1 = 16;
    n12 displayRangeY2 = 16 + 240;

    //GP1(08): display mode
    n3 horizontalResolution;
    n1 verticalResolution;
    n1 videoMode;   //0 = NTSC, 1 = PAL
    n1 colorDepth;  //0 = 15bpp, 1 = 24bpp
    n1 interlace;
    n1 reverseFlag;

    n32 dataReadBuffer;
    n32 vramReadWord;
    n1 vramReadWordValid;
    n1 vramReadLatchValid;

    n32 readTransferFifo[64];
    n7 readTransferFifoLength;

    //internal:
    n6 texturePaletteX;
    n9 texturePaletteY;
    n1 polylineActive;
  } io;

  struct FillState {
    bool active = false;
    bool restricted = false;
    u32 parity = 0;
    u32 x = 0, y = 0, width = 0, height = 0, color = 0;
    u32 clocks = 0, elapsed = 0, written = 0;
  } fill;

  struct Fifo {
    static constexpr u32 Depth = 16;
    static constexpr u32 PtrMask = 31;
    u32 buffer[Depth] = {};
    u8 writePointer = 0;
    u8 readPointer = 0;

    auto empty() const -> bool { return writePointer == readPointer; }
    auto size() const -> u32 { return (writePointer - readPointer) & PtrMask; }
    auto full() const -> bool { return size() >= Depth; }

    auto write(u32 word) -> void {
      if(full()) readPointer = (readPointer + 1) & PtrMask;
      buffer[writePointer & (Depth - 1)] = word;
      writePointer = (writePointer + 1) & PtrMask;
    }

    auto read() -> u32 {
      u32 word = buffer[readPointer & (Depth - 1)];
      readPointer = (readPointer + 1) & PtrMask;
      return word;
    }

    auto reset() -> void {
      writePointer = readPointer = 0;
    }
  } fifo;

  struct FifoIngress {
    u8   length = 0;
    u8   target = 0;

    auto reset() -> void { length = target = 0; }
  } ingress;

  struct CommandLatch {
    n32 data[256];
    n8 length;
    n8 target;
    n1 ready;

    auto reset() -> void { length = target = ready = 0; }
  } latch;

  struct Queue {
    auto reset() -> void { length = counterX = counterY = 0; }
    auto empty() const -> bool { return length == 0; }
    auto write(n32 value) -> n8 { data[length++] = value; return length; }

    n8  command;
    n8  length;
    n32 data[256];
    n10 counterX;
    n9  counterY;
  };

  struct {
    Queue gp0;
    Queue gp1;
  } queue;

  struct Delta { s32 x, y; };
  struct Point { s32 x, y; };
  struct Texel { s32 u, v; };

  struct Color {
    static Color table[65536];

    static auto fromRGB(s32 r, s32 g, s32 b) -> Color {
      return {
        u8(std::clamp(r, 0, 255)),
        u8(std::clamp(g, 0, 255)),
        u8(std::clamp(b, 0, 255))
      };
    }

    static auto from16(u16 data) -> Color {
      return table[data];
    }

    static auto to16(u32 data) -> u16 {
      return (data >> 3 & 0x1f) << 0 | (data >> 11 & 0x1f) << 5 | (data >> 19 & 0x1f) << 10;
    }

    auto to16() const -> u16 {
      return (r >> 3) << 0 | (g >> 3) << 5 | (b >> 3) << 10;
    }

    u8 r, g, b;
  };

  struct Vertex : Point, Texel, Color {
    auto setPoint(u32 data) -> Vertex& {
      x = i11(i12(data >>  0));
      y = i11(i12(data >> 16));
      return *this;
    }

    auto setPoint(s16 px, s16 py) -> Vertex& {
      x = px;
      y = py;
      return *this;
    }

    auto setTexel(u32 data) -> Vertex& {
      u = u8(data >> 0);
      v = u8(data >> 8);
      return *this;
    }

    auto setTexel(s16 tu, s16 tv) -> Vertex& {
      u = tu;
      v = tv;
      return *this;
    }

    auto setColor(u32 data) -> Vertex& {
      r = u8(data >>  0);
      g = u8(data >>  8);
      b = u8(data >> 16);
      return *this;
    }

    auto setColor(u8 cr, u8 cg, u8 cb) -> Vertex& {
      r = cr;
      g = cg;
      b = cb;
      return *this;
    }
  };

  struct Size {
    auto setSize(u32 data) -> Size& {
      w = data >>  0 & 0x3ff;
      h = data >> 16 & 0x1ff;
      return *this;
    }

    auto setSize(u16 sw, u16 sh) -> Size& {
      w = sw;
      h = sh;
      return *this;
    }

    u16 w, h;
  };

  //rendering flags
  enum : u32 {
    None      = 0,
    Raw       = 1 << 0,
    Alpha     = 1 << 1,
    Texture   = 1 << 2,
    Shade     = 1 << 3,
    Dither    = 1 << 4,
    Fill      = 1 << 5,
    Line      = 1 << 6,
    Rectangle = 1 << 7,
  };

  //renderer.cpp
  auto generateTables() -> void;
  auto invalidateTextureCache() -> void;
  auto invalidateCLUTCache() -> void;
  auto invalidateCache() -> void;
  auto queueTextureCacheInvalidation() -> void;
  auto queueCacheInvalidation() -> void;

  struct Render {
    auto weight(Point a, Point b, Point c) const -> s32;
    auto drawableArea(Point a, Point b, Point c) const -> s32;
    auto delta(Point a, Point b, Point c, s32 d[3], s32 area) const -> Delta;
    auto loadCLUTCache() const -> void;
    auto texel(Point p) const -> u16;
    auto dither(Point p, Color c) const -> Color;
    auto modulate(Color above, Color below) const -> Color;
    auto alpha(u16 above, u16 below) const -> u16;
    template<u32 Flags> auto pixel(Point, Color, Point = {}, u16* = nullptr) -> void;
    template<typename Draw> auto writeSpan(s32 x, s32 y, s32 width, Draw&& draw) -> void;
    template<u32 Flags> auto line() -> void;
    template<u32 Flags> auto triangle() -> void;
    template<u32 Flags> auto quadrilateral() -> void;
    template<u32 Flags> auto rectangle() -> void;
    template<u32 Flags> auto fill() -> void;
    template<u32 Flags> auto cost(u32 pixels) const -> u32;
    auto execute() -> void;

    u32  command;
    u32  flags;
    bool dithering;
    bool interlacedRendering;
    bool activeLineLSB;
    u32  semiTransparency;
    bool checkMaskBit;
    bool forceMaskBit;
    s32  drawingAreaOriginX1;
    s32  drawingAreaOriginY1;
    s32  drawingAreaOriginX2;
    s32  drawingAreaOriginY2;
    s32  drawingAreaOffsetX;
    s32  drawingAreaOffsetY;
    u32  textureDepth;
    u32  texturePageBaseX;
    u32  texturePageBaseY;
    u32  texturePaletteX;
    u32  texturePaletteY;
    u32  texelMaskX;
    u32  texelMaskY;
    u32  texelOffsetX;
    u32  texelOffsetY;
    u32  texelErrorMask = 0;
    bool textureFlipX;
    bool textureFlipY;

    Vertex v0;
    Vertex v1;
    Vertex v2;
    Vertex v3;
    Size size;
  };

  auto drawCycles(const Render& render) const -> u32;
  auto drawBusy() const -> bool { return io.pcounter > 0 || pendingTextureTiming; }
  auto finishPendingRender() -> void;

  bool pendingTextureTiming = false;
  u32 textureCyclesAtQueue = 0;
  u32 textureBlendPixelsAtQueue = 0;

//unserialized:
  //renderer.cpp
  struct Renderer {
    GPU& self;
    Renderer(GPU& self) : self(self) {}

    auto queue(Render& render) -> void;
    auto synchronize() -> void;
    auto main(uintptr_t) -> void;
    auto kill() -> void;
    auto power() -> void;

    nall::thread handle;
    queue_spsc<Render[65536]> fifo;
    std::atomic<u32> workSequence = 0;
    std::atomic<u32> completedSequence = 0;
    u32 textureCycles = 0;
    u32 textureBlendPixels = 0;
  } renderer{*this};

  struct TextureCache {
    u32 tag;
    u16 data[4];
  };

  TextureCache textureCache[256];
  u16 clutCache[256];
  u32 clutCacheTag4;
  u32 clutCacheTag8;

  struct Scanout {
    GPU& self;
    Scanout(GPU& self) : self(self) {}

    auto queue() -> void;
    auto lineStart() -> void;
    auto clock(u32 hclock) -> void;
    auto flushPixel(u32 hclock) -> void;
    auto pixel(u32 hclock) -> void;
    auto readRGB24(u32 sourceX) -> u32;
    auto lineEnd() -> void;
    auto renderLine() -> void;
    auto power() -> void;

    static constexpr u32 ClockScale = 4;
    static constexpr u32 OutputPitch = 2812 / ClockScale;
    static constexpr u32 OutputHeight = 576;
    static constexpr u32 LineScale = 2;

    u32  depth;
    u32  dotclock;
    u32  rangeY1;
    u32  rangeY2;
    bool interlace;
    bool interleaved;
    bool pal;
    bool accurate;
    bool disabled;
    u32 rangeX1;
    u32 divider;

    bool lineVisible;
    u32  targetY;
    u32  sourceBase;
    u32  sourceStartX;
    u32  srcPixelX;
    u32* outputPtr;

    bool displayCountLatched;
    bool displayOutputActive;
    u32 displayPixelsRemaining;

    bool  hasPrevPixel;
    u32   prevPixelStart;
    u32   prevPixelColor;

  } scanout{*this};

  Node::Setting::Boolean pixelAccuracySetting;
  bool pixelAccuracy = false;

  u16* vram2D[512];
  u8   ditherTable[4][4][256];
  static constexpr u32 dotclockDividers[5] = {10, 8, 5, 4, 7};
  static constexpr u32 displayWidths[5] = {256, 320, 512, 640, 368};
  bool refreshed;
};

extern GPU gpu;
