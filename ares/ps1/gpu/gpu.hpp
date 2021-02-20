//Graphics Processing Unit

struct GPU : Thread, Memory::Interface {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscan;
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

  auto vtotal() const -> u32 { return io.videoMode ? 314 : 263; }
  auto vstart() const -> u32 { return io.displayRangeY1; }
  auto vend() const -> u32 { return io.displayRangeY2; }
  auto vblank() const -> bool { return io.vcounter < vstart() || io.vcounter >= vend(); }
  auto interlace() const -> bool { return io.verticalResolution && io.interlace; }

  //gpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto frame() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  //io.cpp
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
  auto writeGP0(u32 data, bool isThread = false) -> void;

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
    Status,
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

    n1  field;     //even or odd scanline
    n16 hcounter;  //horizontal counter
    n16 vcounter;  //vertical counter
    i32 pcounter;  //pixel drawing counter

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
    n1 textureDisable;
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
    i11 drawingAreaOffsetX;
    i11 drawingAreaOffsetY;

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
    n3 horizontalResolution = 1;
    n1 verticalResolution;
    n1 videoMode;   //0 = NTSC, 1 = PAL
    n1 colorDepth;  //0 = 15bpp, 1 = 24bpp
    n1 interlace;
    n1 reverseFlag;

    //GP1(10): get GPU information
    n24 status;

    //internal:
    n6 texturePaletteX;
    n9 texturePaletteY;
  } io;

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

  struct Delta { f32 x, y; };
  struct Point { s32 x, y; };
  struct Texel { s32 u, v; };

  struct Color {
    static Color table[65536];

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
      x = i11(data >>  0);
      y = i11(data >> 16);
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
      w = u16(data >>  0);
      h = u16(data >> 16);
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

  struct Render {
    auto weight(Point a, Point b, Point c) const -> s32;
    auto origin(Point a, Point b, Point c, s32 d[3], f32 area, s32 bias[3]) const -> f32;
    auto delta(Point a, Point b, Point c, s32 d[3], f32 area) const -> Delta;
    auto texel(Point p) const -> u16;
    auto dither(Point p, Color c) const -> Color;
    auto modulate(Color above, Color below) const -> Color;
    auto alpha(Color above, Color below) const -> Color;
    template<u32 Flags> auto pixel(Point, Color, Point = {}) -> void;
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
    u32  semiTransparency;
    bool checkMaskBit;
    bool forceMaskBit;
    s32  drawingAreaOriginX1;
    s32  drawingAreaOriginY1;
    s32  drawingAreaOriginX2;
    s32  drawingAreaOriginY2;
    u32  drawingAreaOffsetX;
    u32  drawingAreaOffsetY;
    u32  textureDepth;
    u32  texturePageBaseX;
    u32  texturePageBaseY;
    u32  texturePaletteX;
    u32  texturePaletteY;
    u32  texelMaskX;
    u32  texelMaskY;
    u32  texelOffsetX;
    u32  texelOffsetY;

    Vertex v0;
    Vertex v1;
    Vertex v2;
    Vertex v3;
    Size size;
  };

//unserialized:
  //renderer.cpp
  struct Renderer {
    GPU& self;
    Renderer(GPU& self) : self(self) {}

    auto queue(Render& render) -> void;
    auto main(uintptr_t) -> void;
    auto kill() -> void;
    auto power() -> void;

    nall::thread handle;
    queue_spsc<Render[65536]> fifo;
  } renderer{*this};

  //blitter.cpp
  struct Blitter {
    GPU& self;
    Blitter(GPU& self) : self(self) {}

    auto queue() -> void;
    auto refresh() -> void;
    auto power() -> void;

    bool blank;
    u32  depth;
    u32  width;
    u32  height;
    s32  sx;
    s32  sy;
    s32  tx;
    s32  ty;
    s32  tw;
    s32  th;
  } blitter{*this};

  u16* vram2D[512];
  u8   ditherTable[4][4][256];
  bool refreshed;
};

extern GPU gpu;
