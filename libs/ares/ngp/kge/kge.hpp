//K1GE: K1 Graphics Engine (Neo Geo Pocket)
//K2GE: K2 Graphics Engine (Neo Geo Pocket Color)

struct KGE : Thread {
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
  auto readObject(n8 address) -> n8;
  auto writeObject(n8 address, n8 data) -> void;
  auto readObjectColor(n6 address) -> n8;
  auto writeObjectColor(n6 address, n8 data) -> void;
  auto readColor(n9 address) -> n8;
  auto writeColor(n9 address, n8 data) -> void;
  auto readAttribute(n12 address) -> n8;
  auto writeAttribute(n12 address, n8 data) -> void;
  auto readCharacter(n13 address) -> n8;
  auto writeCharacter(n13 address, n8 data) -> void;

  //color.cpp
  auto colorNeoGeoPocket(n32) -> n64;
  auto colorNeoGeoPocketColor(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Window {
    KGE& self;

    struct Output {
      n3 color;
    } output;

    //window.cpp
    auto render(n8 x, n8 y) -> maybe<Output&>;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 hoffset;
    n8 voffset;
    n8 hlength;
    n8 vlength;
    n3 color;  //color drawn outside of window
  } window{*this};

  struct Plane {
    KGE& self;

    struct Output {
      n6 color;
      n1 priority;
    } output;

    //plane.cpp
    auto render(n8 x, n8 y) -> maybe<Output&>;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 hscroll;
    n8 vscroll;
    n3 palette[2][4];
  } plane1{*this}, plane2{*this};

  struct Sprite {
    KGE& self;

    struct Output {
      n6 color;
      n2 priority;
    } output;

    //sprite.cpp
    auto begin(n8 y) -> void;
    auto render(n8 x, n8 y) -> maybe<Output&>;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
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
    } objects[64];

    struct Tile {
      n8 x;
      n3 y;
      n9 character;
      n2 priority;
      n1 palette;
      n1 hflip;
      n4 code;  //K2GE
    } tiles[64];

    n8 hscroll;
    n8 vscroll;
    n3 palette[2][4];
  } sprite{*this};

  struct DAC {
    KGE& self;

    //dac.cpp
    auto begin(n8 y) -> void;
    auto run(n8 x, n8 y) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n12 colors[256];  //K2GE

    n1  colorMode;  //0 = K2GE, 1 = K1GE compatible
    n1  negate;     //0 = normal, 1 = inverted display

  //unserialized:
    u32* output = nullptr;
  } dac{*this};

  struct Attribute {
    n9 character;
    n4 code;  //K2GE
    n1 palette;
    n1 vflip;
    n1 hflip;
  } attributes[2048];

  n2 characters[512][8][8];

  struct Background {
    n3 color;
    n3 unused;
    n2 mode;
  } background;

  struct LED {
    n8 control = 0x07;
    n8 frequency = 0x80;
  } led;

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

extern KGE kge;
