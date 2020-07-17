//Graphics Processing Unit

struct GPU : Thread {
  Node::Component node;
  Node::Screen screen;
  Memory::Writable vram;

  //gpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u8;
  auto readHalf(u32 address) -> u16;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u8 data) -> void;
  auto writeHalf(u32 address, u16 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //gp0.cpp
  auto readGP0() -> u32;
  auto writeGP0(u32 data) -> void;

  //gp1.cpp
  auto readGP1() -> u32;
  auto writeGP1(u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  enum class Mode : uint {
    Normal,
    CopyToVRAM,
    CopyFromVRAM,
  };

  struct IO {
    Mode mode = Mode::Normal;

     uint1 field = 0;
    uint16 vcounter = 0;

    //GP0(a0): copy rectangle (CPU to VRAM)
    //GP0(c0): copy rectangle (VRAM to CPU)
    struct Copy {
      uint16 x;
      uint16 y;
      uint16 width;
      uint16 height;
    //internal:
      uint16 px;
      uint16 py;
    } copy;

    //GP0(e1): draw mode
     uint4 texturePageBaseX = 0;
     uint1 texturePageBaseY = 0;
     uint2 semiTransparency = 0;
     uint2 textureDepth = 0;
     uint1 dithering = 0;
     uint1 drawToDisplay = 0;
     uint1 textureDisable = 0;
     uint1 textureFlipX = 0;
     uint1 textureFlipY = 0;

    //GP0(e2): texture window
     uint5 textureWindowMaskX = 0;
     uint5 textureWindowMaskY = 0;
     uint5 textureWindowOffsetX = 0;
     uint5 textureWindowOffsetY = 0;

    //GP0(e3): set drawing area (top left)
    uint10 drawingAreaOriginX1 = 0;
    uint10 drawingAreaOriginY1 = 0;

    //GP0(e4): set drawing area (bottom right)
    uint10 drawingAreaOriginX2 = 0;
    uint10 drawingAreaOriginY2 = 0;

    //GP0(e5): set drawing offset
     int11 drawingAreaOffsetX = 0;
     int11 drawingAreaOffsetY = 0;

    //GP0(e6): mask bit
     uint1 forceMaskBit = 0;
     uint1 checkMaskBit = 0;

    //GP1(02): acknowledge interrupt
     uint1 interrupt = 0;

    //GP1(03): display disable
     uint1 displayDisable = 1;

    //GP1(04): DMA direction
     uint2 dmaDirection = 0;

    //GP1(05): start of display area
    uint10 displayStartX = 0;
     uint9 displayStartY = 0;

    //GP1(06): horizontal display range
    uint12 displayRangeX1 = 0;
    uint12 displayRangeX2 = 0;

    //GP1(07): vertical display range
    uint12 displayRangeY1 = 0;
    uint12 displayRangeY2 = 0;

    //GP1(08): display mode
     uint3 horizontalResolution = 0;
     uint1 verticalResolution = 0;
     uint1 videoMode = 0;
     uint1 colorDepth = 0;
     uint1 interlace = 0;
     uint1 reverseFlag = 0;

    //internal:
     uint6 texturePaletteX;
     uint9 texturePaletteY;
  } io;

  struct Queue {
    auto reset() -> void { length = counterX = counterY = 0; }
    auto empty() const -> bool { return length == 0; }
    auto write(uint32 value) -> uint8 { data[length++] = value; return length; }

     uint8 command;
     uint8 length;
    uint32 data[256];
    uint10 counterX;
     uint9 counterY;
  };

  struct {
    Queue gp0;
    Queue gp1;
  } queue;

  struct Point { i32 x, y; };
  struct Texel { i32 u, v; };

  struct Color {
    static auto from16(u16 data) -> Color {
      return {
        (data >>  0 & 31) << 3 | (data >>  0 & 31) >> 2,
        (data >>  5 & 31) << 3 | (data >>  5 & 31) >> 2,
        (data >> 10 & 31) << 3 | (data >> 10 & 31) >> 2
      };
    }

    static auto to16(u32 data) -> u16 {
      return (data >> 3 & 0x1f) << 0 | (data >> 11 & 0x1f) << 5 | (data >> 19 & 0x1f) << 10;
    }

    auto to16() const -> u16 {
      return {(r >> 3) << 0 | (g >> 3) << 5 | (b >> 3) << 10};
    }

    u8 r, g, b;
  };

  struct Vertex : Point, Texel, Color {
    auto setPoint(u32 data) -> Vertex& {
      x = i16(data >>  0);
      y = i16(data >> 16);
      return *this;
    }

    auto setPoint(i16 px, i16 py) -> Vertex& {
      x = px;
      y = py;
      return *this;
    }

    auto setTexel(u32 data) -> Vertex& {
      u = u8(data >> 0);
      v = u8(data >> 8);
      return *this;
    }

    auto setTexel(i16 tu, i16 tv) -> Vertex& {
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

  struct Render {
    enum : uint { Color         = 1 << 0 };
    enum : uint { Shade         = 1 << 1 };
    enum : uint { Texel         = 1 << 2 };
    enum : uint { Alpha         = 1 << 3 };
    enum : uint { ModulateColor = 1 << 4 };
    enum : uint { ModulateShade = 1 << 5 };
  };

  //render.cpp
  auto weight(Point a, Point b, Point c) const -> i32;
  auto texel(Point p) const -> u16;
  auto dither(Point p, Color c) const -> Color;
  auto renderPixelColor(Point p, Color c) -> void;
  auto renderPixelAlpha(Point P, Color c) -> void;
  auto renderSolidLine(Point p0, Point p1, Color c) -> void;
  template<uint Flags> auto renderTriangle(Vertex v0, Vertex v1, Vertex v2) -> void;
  template<uint Flags> auto renderQuadrilateral(Vertex v0, Vertex v1, Vertex v2, Vertex v3) -> void;
  template<uint Flags> auto renderRectangle(Vertex v0, Size sz) -> void;

//unserialized:
  u32 output[1024 * 512];
  bool refreshed;
};

extern GPU gpu;
