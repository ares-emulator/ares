//K1GE: K1 Graphics Engine (Neo Geo Pocket)
//K2GE: K2 Graphics Engine (Neo Geo Pocket Color)

struct VPU : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean interframeBlending;

  //vpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  //memory.cpp
  auto read(n24 address) -> n8;
  auto write(n24 address, n8 data) -> void;
  auto readSprite(n8 address) -> n8;
  auto writeSprite(n8 address, n8 data) -> void;
  auto readSpriteColor(n6 address) -> n8;
  auto writeSpriteColor(n6 address, n8 data) -> void;
  auto readColor(n9 address) -> n8;
  auto writeColor(n9 address, n8 data) -> void;
  auto readAttribute(n12 address) -> n8;
  auto writeAttribute(n12 address, n8 data) -> void;
  auto readCharacter(n13 address) -> n8;
  auto writeCharacter(n13 address, n8 data) -> void;

  //window.cpp
  auto renderWindow(n8 x, n8 y) -> bool;

  //plane.cpp
  struct Plane;
  auto renderPlane(n8 x, n8 y, Plane&) -> bool;

  //sprite.cpp
  auto cacheSprites(n8 y) -> void;
  auto renderSprite(n8 x) -> bool;

  //color.cpp
  auto colorNeoGeoPocket(n32) -> n64;
  auto colorNeoGeoPocketColor(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  n12 colors[256];

  struct Background {
    n3 color;   //K2GE
    n3 unused;  //K2GE
    n2 mode;    //K2GE
  } background;

  struct Window {
    n8 hoffset;
    n8 voffset;
    n8 hlength;
    n8 vlength;
    n3 color;  //color drawn outside of window

    n12 output;
  } window;

  struct Attribute {
    n9 character;
    n4 code;  //K2GE
    n1 palette;
    n1 vflip;
    n1 hflip;
  } attributes[2048];

  n2 characters[512][8][8];

  struct Plane {
    n12 address;
    n8  colorNative;      //K2GE
    n8  colorCompatible;  //K2GE
    n8  hscroll;
    n8  vscroll;
    n3  palette[2][4];

    n12 output;
    n1  priority;
  } plane1, plane2;

  struct Sprites {
    n8 colorNative;      //K2GE
    n8 colorCompatible;  //K2GE
    n8 hscroll;
    n8 vscroll;
    n3 palette[2][4];

    n12 output;
    n2  priority;
  } sprite;

  struct Sprite {
    n9 character;
    n1 vchain;
    n1 hchain;
    n2 priority;
    n1 palette;
    n1 vflip;
    n1 hflip;
    n8 hoffset;
    n8 voffset;
    n4 code;  //K2GE
  } sprites[64];

  struct Tile {
    n8 x;
    n8 y;
    n9 character;
    n2 priority;
    n1 palette;
    n1 hflip;
    n4 code;  //K2GE
  } tiles[64];

  n8 tileCount;

  struct LED {
    n8 control = 0x07;
    n8 frequency = 0x80;
  } led;

  struct DAC {
    n1 negate;     //0 = normal, 1 = inverted display
    n1 colorMode;  //0 = K2GE, 1 = K1GE compatible
  } dac;

  struct IO {
    n8  vlines = 0xc6;
    n8  vcounter;
    n10 hcounter;

    n1 hblankEnableIRQ = 1;  //todo: should be 0
    n1 vblankEnableIRQ = 1;
    n1 hblankActive;
    n1 vblankActive;

    n1 characterOver;

    n1 planePriority;  //0 = plane1 > plane2; 1 = plane2 > plane1
  } io;
};

extern VPU vpu;
