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
  auto gp0(u32 data) -> void;

  //gp1.cpp
  auto gp1(u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint1 field = 0;
    uint16 vcounter = 0;

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

/*
  struct Point {
    static auto fromData(u32 data) -> Point { return {i16(data >> 0), i16(data >> 16)}; }
    i32 x, y;
  };

  struct Color {
    static auto fromData(u32 data) -> Color { return {u8(data >> 0), u8(data >> 8), u8(data >> 16)}; }
    u8 r, g, b;
  };

  struct Texel {
    static auto fromData(u32 data) -> Texel { return {u8(data >> 0), u8(data >> 8)}; }
    i32 u, v;
  };

  struct Palette {
    static auto fromData(u32 data) -> Palette { return {(data >> 16 & 0x3f) << 4, data >> 22 & 0x1ff}; }
    u16 px, py;
  };

  struct Page {
    static auto fromData(u32 data) -> Page { return {(data >> 16 & 15) * 64, (data >> 20 & 1) * 256}; }
    u16 pageX, pageY;
  };

  struct TexelState : Color, Palette, Page {
    static auto fromData(u32 color, u32 palette, u32 page) -> TexelState {
      return {Color::fromData(color), Palette::fromData(palette), Page::fromData(page)};
    }
  };

  struct ColorPoint : Point, Color {
    static auto fromData(u32 point, u32 color) -> ColorPoint {
      return {Point::fromData(point), Color::fromData(color)};
    }
  };

  struct TexelPoint : Point, Texel {
    static auto fromData(u32 point, u32 texel) -> TexelPoint {
      return {Point::fromData(point), Texel::fromData(texel)};
    }
  };
*/

  struct Point { i32 x, y; };
  struct Texel { i32 u, v; };
  struct Color {  u8 r, g, b; };

  struct Vertex : Point, Texel, Color {
    auto setPoint(u32 data) -> Vertex& {
      x = i16(data >>  0);
      y = i16(data >> 16);
      return *this;
    }

    auto setTexel(u32 data) -> Vertex& {
      u = u8(data >> 0);
      v = u8(data >> 8);
      return *this;
    }

    auto setColor(u32 data) -> Vertex& {
      r = u8(data >>  0);
      g = u8(data >>  8);
      b = u8(data >> 16);
      return *this;
    }
  };

  struct Palette { u16 cx, cy; };
  struct Page { u16 px, py; };

  struct Texture : Palette, Page {
    auto setPalette(u32 data) -> Texture& {
      cx = (data >> 16 &  0x3f) << 4;
      cy = (data >> 22 & 0x1ff) << 0;
      return *this;
    }

    auto setPage(u32 data) -> Texture& {
      px = (data >> 16 & 0xf) << 6;
      py = (data >> 20 & 0x1) << 8;
      return *this;
    }
  };

  struct Render {
    enum : uint { Solid = 1 << 0 };
    enum : uint { Color = 1 << 1 };
    enum : uint { Texel = 1 << 2 };
  };

  //render.cpp
  auto weight(Point a, Point b, Point c) const -> i32;
  auto renderPixel(Point p, Color c) -> void;
  auto renderSolidLine(Point p0, Point p1, Color c) -> void;
  template<uint RenderFlags = 0> auto renderTriangle(Vertex v0, Vertex v1, Vertex v2, Texture ts) -> void;

//unserialized:
  u32 output[1024 * 512];
  bool refreshed;
};

extern GPU gpu;
