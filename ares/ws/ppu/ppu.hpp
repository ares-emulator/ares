struct PPU : Thread, IO {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean colorEmulation;
  Node::Setting::Boolean interframeBlending;
  Node::Setting::String orientation;
  Node::Setting::Boolean showIcons;
  struct Icon {
    Node::Video::Sprite auxiliary0;
    Node::Video::Sprite auxiliary1;
    Node::Video::Sprite auxiliary2;
    Node::Video::Sprite headphones;
    Node::Video::Sprite initialized;
    Node::Video::Sprite lowBattery;
    Node::Video::Sprite orientation0;
    Node::Video::Sprite orientation1;
    Node::Video::Sprite poweredOn;
    Node::Video::Sprite sleeping;
    Node::Video::Sprite volumeA0;
    Node::Video::Sprite volumeA1;
    Node::Video::Sprite volumeA2;
    Node::Video::Sprite volumeB0;
    Node::Video::Sprite volumeB1;
    Node::Video::Sprite volumeB2;
    Node::Video::Sprite volumeB3;
  } icon;

  auto hcounter() const -> u32 { return io.hcounter; }
  auto vcounter() const -> u32 { return io.vcounter; }
  auto field() const -> bool { return io.field; }

  auto planar() const -> bool { return system.mode().bit(0) == 0; }
  auto packed() const -> bool { return system.mode().bit(0) == 1; }
  auto depth() const -> u32 { return system.mode().bit(1,2) != 3 ? 2 : 4; }
  auto grayscale() const -> bool { return system.mode().bit(1,2) == 0; }

  //ppu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto scanline() -> void;
  auto frame() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;
  auto updateIcons() -> void;
  auto updateOrientation() -> void;

  //io.cpp
  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  //memory.cpp
  auto fetch(n10 tile, n3 x, n3 y) -> n4;
  auto backdrop(n8 color) -> n12;
  auto palette(n4 palette, n4 color) -> n12;
  auto opaque(n4 palette, n4 color) -> n1;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Output {
    n1  valid;
    n12 color;
  };

  struct Window {
    //window.cpp
    auto scanline(n8 y) -> void;
    auto inside(n8 x, n8 y) const -> bool;
    auto outside(n8 x, n8 y) const -> bool;
    auto power() -> void;

    n1 enable[2];
    n1 invert[2];  //Screen2 only
    n8 x0[2];
    n8 x1[2];
    n8 y0[2];
    n8 y1[2];
  };

  struct Screen {
    PPU& self;

    //screen.cpp
    auto scanline(n8 y) -> void;
    auto pixel(n8 x, n8 y) -> void;
    auto power() -> void;

    n1 enable[2];
    n4 mapBase[2];
    n8 hscroll[2];
    n8 vscroll[2];

    Output output;
  };

  struct Screen1 : Screen {
    //screen.cpp
    auto scanline(n8 y) -> void;
    auto pixel(n8 x, n8 y) -> void;
    auto power() -> void;
  } screen1{*this};

  struct Screen2 : Screen {
    //screen.cpp
    auto scanline(n8 y) -> void;
    auto pixel(n8 x, n8 y) -> void;
    auto power() -> void;

    Window window;
  } screen2{*this};

  struct Sprite {
    PPU& self;
    Screen2& screen2;

    //sprite.cpp
    auto frame() -> void;
    auto scanline(n8 y) -> void;
    auto pixel(n8 x, n8 y) -> void;
    auto power() -> void;

    n1 enable[2];
    n6 oamBase;
    n7 first;
    n8 count;  //0 - 128
    n8 valid;  //0 - 32

    Output output;
    Window window;

    queue<n32[128]> oam[2];
    queue<n32[32]> objects;
  } sprite{*this, screen2};

  struct DAC {
    PPU& self;
    Screen1& screen1;
    Screen2& screen2;
    Sprite& sprite;

    //dac.cpp
    auto scanline(n8 y) -> void;
    auto pixel(n8 x, n8 y) -> void;
    auto power() -> void;

    n1 enable[2];
    n1 contrast[2];
    n8 unknown;
    n8 backdrop[2];

  //unserialized:
    u32* output = nullptr;
  } dac{*this, screen1, screen2, sprite};

  struct PRAM {
    n4 pool[8];
    struct Palette {
      n3 color[4];
    } palette[16];
  } pram;

  struct LCD {
    struct Icon {
      n1 sleeping;
      n1 orientation1;
      n1 orientation0;
      n1 auxiliary0;
      n1 auxiliary1;
      n1 auxiliary2;
    } icon;
  } lcd;

  struct Timer {
    //timer.cpp
    auto step() -> bool;

    n1  enable;
    n1  repeat;
    n16 frequency;
    n16 counter;
  } htimer, vtimer;

  struct IO {
    n8 hcounter;
    n8 vcounter;
    n8 vsync  = 155;
    n8 vtotal = 158;
    n8 vcompare;
    n1 field;
    n1 orientation;
  } io;
};

extern PPU ppu;
