#pragma once

namespace ares {

//Yamaha V9938

struct V9938 {
  Node::Video::Screen screen;
  Memory::Writable<n8> vram;  //video RAM
  Memory::Writable<n8> xram;  //expansion RAM
  Memory::Writable<n9> pram;  //palette RAM

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto irq(bool line) -> void = 0;
  virtual auto frame() -> void = 0;

  auto videoMode() const -> n5 { return io.videoMode; }
  auto vcounter() const -> n16 { return io.vcounter; }
  auto hcounter() const -> n16 { return io.hcounter; }

  auto timing() const -> bool { return latch.timing; }
  auto overscan() const -> bool { return latch.overscan; }
  auto interlace() const -> bool { return latch.interlace; }
  auto field() const -> bool { return latch.field; }

  auto vtotal() const -> u32 { return !latch.timing   ? 262 : 313; }
  auto vlines() const -> u32 { return !latch.overscan ? 192 : 212; }

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

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Background {
    V9938& self;

    //background.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto text1(n8 hoffset, n8 voffset) -> void;
    auto text2(n8 hoffset, n8 voffset) -> void;
    auto graphic1(n8 hoffset, n8 voffset) -> void;
    auto graphic2(n8 hoffset, n8 voffset) -> void;
    auto graphic3(n8 hoffset, n8 voffset) -> void;
    auto graphic4(n8 hoffset, n8 voffset) -> void;
    auto graphic5(n8 hoffset, n8 voffset) -> void;
    auto graphic6(n8 hoffset, n8 voffset) -> void;
    auto graphic7(n8 hoffset, n8 voffset) -> void;
    auto multicolor(n8 hoffset, n8 voffset) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n17 nameTableAddress;
      n17 patternTableAddress;
      n17 colorTableAddress;
      i4 hadjust;  //todo
      i4 vadjust;  //todo
      n8 vscroll;  //todo
    } io;

    struct Latch {
      i4 hadjust;
      i4 vadjust;
      n8 vscroll;
    } latch;

    struct Output {
      n4 color;
    } output;
  } background{*this};

  struct Sprite {
    V9938& self;

    //sprite.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto sprite1(n8 hoffset, n8 voffset) -> void;
    auto sprite2(n8 hoffset, n8 voffset) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
      i9 x;
      i9 y = 0xd0;
      n8 pattern;
      n4 color;
      n1 collision;
      n1 priority;
    } objects[8];

    struct IO {
      n17 nameTableAddress;
      n17 patternTableAddress;

      n1 zoom;
      n1 size;  //0 = 8x8, 1 = 16x16
      n1 disable;

      //flags
      n5 overflowIndex;
      n1 overflow;
      n1 collision;
    } io;

    struct Output {
      n4 color;
    } output;
  } sprite{*this};

  struct DAC {
    V9938& self;

    //dac.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 enable;
      n1 digitize;   //todo
      n1 grayscale;  //todo
    } io;

  //unserialized:
    u32* output = nullptr;
  } dac{*this};

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

  struct IO {
    n16 vcounter;
    n16 hcounter;

    n1  controlLatch;
    n16 controlValue;

    n5  videoMode;
    n1  timing;     //0 = 262, 1 = 313
    n1  interlace;  //0 = progressive, 1 = interlaced
    n1  overscan;   //0 = 192, 1 = 212

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

  struct Latch {
    n1 timing;
    n1 interlace;
    n1 overscan;
    n1 field;
  } latch;
};

}
