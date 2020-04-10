//K1GE: K1 Graphics Engine (Neo Geo Pocket)
//K2GE: K2 Graphics Engine (Neo Geo Pocket Color)

struct VPU : Thread {
  Node::Component node;
  Node::Screen screen;
  Node::Boolean interframeBlending;

  //vpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
  auto power() -> void;

  //memory.cpp
  auto read(uint24 address) -> uint8;
  auto write(uint24 address, uint8 data) -> void;
  auto readSprite(uint8 address) -> uint8;
  auto writeSprite(uint8 address, uint8 data) -> void;
  auto readSpriteColor(uint6 address) -> uint8;
  auto writeSpriteColor(uint6 address, uint8 data) -> void;
  auto readColor(uint9 address) -> uint8;
  auto writeColor(uint9 address, uint8 data) -> void;
  auto readAttribute(uint12 address) -> uint8;
  auto writeAttribute(uint12 address, uint8 data) -> void;
  auto readCharacter(uint13 address) -> uint8;
  auto writeCharacter(uint13 address, uint8 data) -> void;

  //window.cpp
  auto renderWindow(uint8 x, uint8 y) -> bool;

  //plane.cpp
  struct Plane;
  auto renderPlane(uint8 x, uint8 y, Plane&) -> bool;

  //sprite.cpp
  auto cacheSprites(uint8 y) -> void;
  auto renderSprite(uint8 x) -> bool;

  //color.cpp
  auto colorNeoGeoPocket(uint32) -> uint64;
  auto colorNeoGeoPocketColor(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  uint32 buffer[160 * 152];
  uint12 colors[256];

  struct Background {
    uint3 color;   //K2GE
    uint3 unused;  //K2GE
    uint2 mode;    //K2GE
  } background;

  struct Window {
    uint8 hoffset;
    uint8 voffset;
    uint8 hlength;
    uint8 vlength;
    uint3 color;  //color drawn outside of window

    uint12 output;
  } window;

  struct Attribute {
    uint9 character;
    uint4 code;  //K2GE
    uint1 palette;
    uint1 vflip;
    uint1 hflip;
  } attributes[2048];

  uint2 characters[512][8][8];

  struct Plane {
    uint12 address;
    uint8  colorNative;      //K2GE
    uint8  colorCompatible;  //K2GE
    uint8  hscroll;
    uint8  vscroll;
    uint3  palette[2][4];

    uint12 output;
    uint1  priority;
  } plane1, plane2;

  struct Sprites {
    uint8 colorNative;      //K2GE
    uint8 colorCompatible;  //K2GE
    uint8 hscroll;
    uint8 vscroll;
    uint3 palette[2][4];

    uint12 output;
    uint2  priority;
  } sprite;

  struct Sprite {
    uint9 character;
    uint1 vchain;
    uint1 hchain;
    uint2 priority;
    uint1 palette;
    uint1 vflip;
    uint1 hflip;
    uint8 hoffset;
    uint8 voffset;
    uint4 code;  //K2GE
  } sprites[64];

  struct Tile {
    uint8 x;
    uint8 y;
    uint9 character;
    uint2 priority;
    uint1 palette;
    uint1 hflip;
    uint4 code;  //K2GE
  } tiles[64];

  uint8 tileCount;

  struct LED {
    uint8 control = 0x07;
    uint8 frequency = 0x80;
  } led;

  struct DAC {
    uint1 negate;     //0 = normal, 1 = inverted display
    uint1 colorMode;  //0 = K2GE, 1 = K1GE compatible
  } dac;

  struct IO {
    uint8  vlines = 0xc6;
    uint8  vcounter;
    uint10 hcounter;

    uint1 hblankEnableIRQ = 1;  //todo: should be 0
    uint1 vblankEnableIRQ = 1;
    uint1 hblankActive;
    uint1 vblankActive;

    uint1 characterOver;

    uint1 planePriority;  //0 = plane1 > plane2; 1 = plane2 > plane1
  } io;
};

extern VPU vpu;
