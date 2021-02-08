#pragma once

namespace ares {

//Yamaha V9938

struct V9938 {
  Node::Video::Screen screen_;

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto irq(bool line) -> void = 0;
  virtual auto frame() -> void = 0;

  auto timing() const -> bool { return latch.timing; }
  auto overscan() const -> bool { return latch.overscan; }
  auto interlace() const -> bool { return latch.interlace; }
  auto field() const -> bool { return latch.field; }

  auto vtotal() const -> u32 { return !latch.timing ? 262 : 313; }
  auto vlines() const -> u32 { return !latch.overscan ? 192 : 212; }

  auto t1() const -> bool { return screen.mode == 0b00001; }
  auto t2() const -> bool { return screen.mode == 0b01001; }
  auto mc() const -> bool { return screen.mode == 0b00010; }
  auto g1() const -> bool { return screen.mode == 0b00000; }
  auto g2() const -> bool { return screen.mode == 0b00100; }
  auto g3() const -> bool { return screen.mode == 0b01000; }
  auto g4() const -> bool { return screen.mode == 0b01100; }
  auto g5() const -> bool { return screen.mode == 0b10000; }
  auto g6() const -> bool { return screen.mode == 0b10100; }
  auto g7() const -> bool { return screen.mode == 0b11100; }

  auto s1() const -> bool { return mc() || g1() || g2(); }
  auto s2() const -> bool { return g3() || g4() || g5() || g6() || g7(); }

  //v9938.cpp
  auto load(Node::Video::Screen) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto poll() -> void;
  auto tick(u32 clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto status() -> n8;
  auto data() -> n8;

  auto data(n8 data) -> void;
  auto control(n8 data) -> void;
  auto palette(n8 data) -> void;
  auto register(n8 data) -> void;

  auto register(n6 register, n8 data) -> void;

  //commands.cpp
  auto command(n8 data) -> void;
  auto command() -> void;
  auto relatch() -> void;
  auto advance() -> bool;
  auto address(n9 x, n10 y) -> n17;
  auto read(n1 source, n9 x, n10 y) -> n8;
  auto write(n1 source, n9 x, n10 y, n8 data) -> void;
  auto logic(n8 dc, n8 sc) -> n8;
  auto point() -> void;
  auto pset() -> void;
  auto srch() -> void;
  auto line() -> void;
  auto lmmv() -> void;
  auto lmmm() -> void;
  auto lmcm() -> void;
  auto lmmc() -> void;
  auto hmmv() -> void;
  auto hmmm() -> void;
  auto ymmm() -> void;
  auto hmmc() -> void;

  //graphic1.cpp
  auto graphic1(n4& color, n8 hoffset, n8 voffset) -> void;

  //graphic2.cpp
  auto graphic2(n4& color, n8 hoffset, n8 voffset) -> void;

  //graphic3.cpp
  auto graphic3(n4& color, n8 hoffset, n8 voffset) -> void;

  //graphic4.cpp
  auto graphic4(n4& color, n8 hoffset, n8 voffset) -> void;

  //sprite1.cpp
  auto sprite1(n8 voffset) -> void;
  auto sprite1(n4& color, n8 hoffset, n8 voffset) -> void;

  //sprite2.cpp
  auto sprite2(n8 voffset) -> void;
  auto sprite2(n4& color, n8 hoffset, n8 voffset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  Memory::Writable<n8> videoRAM;
  Memory::Writable<n8> expansionRAM;
  n9 paletteRAM[16];

  struct VerticalScanIRQ {
    n1 enable;
    n1 pending;
  } virq;

  struct HorizontalScanIRQ {
    n1 enable;
    n1 pending;
    n8 coincidence;
  } hirq;

  struct LightPenIRQ {
    n1 enable;
    n1 pending;
  } lirq;

  struct Latch {
    n1 timing;
    n1 overscan;
    n1 interlace;
    n1 field;
  } latch;

  struct Screen {
    n1 enable;
    n1 digitize;   //unemulated
    n1 grayscale;  //unemulated
    n5 mode;
    n1 interlace;
    n1 overscan;   //0 = 192, 1 = 212
    n1 timing;     //0 = 262, 1 = 313
    i4 hadjust;
    i4 vadjust;
    n8 vscroll;
  } screen;

  struct Table {
    n17 patternLayout;
    n17 patternGenerator;
    n17 spriteAttribute;
    n17 spritePatternGenerator;
    n17 color;
  } table;

  struct SpriteIO {
    n1 magnify;
    n1 size;  //0 = 8x8, 1 = 16x16
    n1 disable;

    n1 collision;
    n1 overflow;
    n5 last;
  } sprite;

  struct SpriteObject {
    n8 x;
    n8 y;
    n8 pattern;
    n4 color;
    n1 collision;
    n1 priority;
  } sprites[8];

  struct IO {
    n16 vcounter;
    n16 hcounter;

    n1  controlLatch;
    n16 controlValue;

    n4  colorBackground;
    n4  colorForeground;

    n4  blinkColorBackground;
    n4  blinkColorForeground;

    n4  blinkPeriodBackground;
    n4  blinkPeriodForeground;

    n4  statusIndex;

    n4  paletteIndex;
    n1  paletteLatch;
    n16 paletteValue;

    n6  registerIndex;
    n1  registerFixed;  //0 = auto-increment

    n1  ramSelect;  //0 = video RAM, 1 = expansion RAM
    n3  ramBank;
    n8  ramLatch;
  } io;

  struct Operation {
    //a very rough, inaccurate approximation of command timing
    n32 counter;

    n1  executing;
    n1  ready;
    n4  command;

    n1  mxs;
    n9  sx;
    n10 sy;

    n1  mxd;
    n9  dx;
    n10 dy;

    n9  nx;
    n10 ny;

    n1  dix;  //0 = right, 1 = left
    n1  diy;  //0 = down, 1 = up

    n8  cr;
    n9  match;
    n1  found;
    n1  eq;
    n1  maj;

    n4  logic;

    //internal latches
    n3  lgm;
    n9  lsx;
    n9  ldx;
    n9  lnx;
  } op;
};

}
