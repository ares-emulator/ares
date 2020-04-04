#pragma once

namespace ares {

//Yamaha V9938

struct V9938 {
  virtual auto step(uint clocks) -> void = 0;
  virtual auto irq(bool line) -> void = 0;
  virtual auto frame() -> void = 0;

  auto timing() const -> bool { return latch.timing; }
  auto overscan() const -> bool { return latch.overscan; }
  auto interlace() const -> bool { return latch.interlace; }
  auto field() const -> bool { return latch.field; }

  auto vtotal() const -> uint { return !latch.timing ? 262 : 313; }
  auto vlines() const -> uint { return !latch.overscan ? 192 : 212; }

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
  auto main() -> void;
  auto poll() -> void;
  auto tick(uint clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto status() -> uint8;
  auto data() -> uint8;

  auto data(uint8 data) -> void;
  auto control(uint8 data) -> void;
  auto palette(uint8 data) -> void;
  auto register(uint8 data) -> void;

  auto register(uint6 register, uint8 data) -> void;

  //commands.cpp
  auto command(uint8 data) -> void;
  auto command() -> void;
  auto relatch() -> void;
  auto advance() -> bool;
  auto address(uint9 x, uint10 y) -> uint17;
  auto read(uint1 source, uint9 x, uint10 y) -> uint8;
  auto write(uint1 source, uint9 x, uint10 y, uint8 data) -> void;
  auto logic(uint8 dc, uint8 sc) -> uint8;
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
  auto graphic1(uint4& color, uint8 hoffset, uint8 voffset) -> void;

  //graphic2.cpp
  auto graphic2(uint4& color, uint8 hoffset, uint8 voffset) -> void;

  //graphic3.cpp
  auto graphic3(uint4& color, uint8 hoffset, uint8 voffset) -> void;

  //graphic4.cpp
  auto graphic4(uint4& color, uint8 hoffset, uint8 voffset) -> void;

  //sprite1.cpp
  auto sprite1(uint8 voffset) -> void;
  auto sprite1(uint4& color, uint8 hoffset, uint8 voffset) -> void;

  //sprite2.cpp
  auto sprite2(uint8 voffset) -> void;
  auto sprite2(uint4& color, uint8 hoffset, uint8 voffset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  uint32 buffer[512 * 424];
  Memory::Writable<uint8> videoRAM;
  Memory::Writable<uint8> expansionRAM;
  uint9 paletteRAM[16];

  struct VerticalScanIRQ {
    uint1 enable;
    uint1 pending;
  } virq;

  struct HorizontalScanIRQ {
    uint1 enable;
    uint1 pending;
    uint8 coincidence;
  } hirq;

  struct LightPenIRQ {
    uint1 enable;
    uint1 pending;
  } lirq;

  struct Latch {
    uint1 timing;
    uint1 overscan;
    uint1 interlace;
    uint1 field;
  } latch;

  struct Screen {
    uint1 enable;
    uint1 digitize;   //unemulated
    uint1 grayscale;  //unemulated
    uint5 mode;
    uint1 interlace;
    uint1 overscan;   //0 = 192, 1 = 212
    uint1 timing;     //0 = 262, 1 = 313
     int4 hadjust;
     int4 vadjust;
    uint8 vscroll;
  } screen;

  struct Table {
    uint17 patternLayout;
    uint17 patternGenerator;
    uint17 spriteAttribute;
    uint17 spritePatternGenerator;
    uint17 color;
  } table;

  struct SpriteIO {
    uint1 magnify;
    uint1 size;  //0 = 8x8, 1 = 16x16
    uint1 disable;

    uint1 collision;
    uint1 overflow;
    uint5 last;
  } sprite;

  struct SpriteObject {
    uint8 x;
    uint8 y;
    uint8 pattern;
    uint4 color;
    uint1 collision;
    uint1 priority;
  } sprites[8];

  struct IO {
    uint16 vcounter;
    uint16 hcounter;

     uint1 controlLatch;
    uint16 controlValue;

     uint4 colorBackground;
     uint4 colorForeground;

     uint4 blinkColorBackground;
     uint4 blinkColorForeground;

     uint4 blinkPeriodBackground;
     uint4 blinkPeriodForeground;

     uint4 statusIndex;

     uint4 paletteIndex;
     uint1 paletteLatch;
    uint16 paletteValue;

     uint6 registerIndex;
     uint1 registerFixed;  //0 = auto-increment

     uint1 ramSelect;  //0 = video RAM, 1 = expansion RAM
     uint3 ramBank;
     uint8 ramLatch;
  } io;

  struct Operation {
    //a very rough, inaccurate approximation of command timing
    uint32 counter;

     uint1 executing;
     uint1 ready;
     uint4 command;

     uint1 mxs;
     uint9 sx;
    uint10 sy;

     uint1 mxd;
     uint9 dx;
    uint10 dy;

     uint9 nx;
    uint10 ny;

     uint1 dix;  //0 = right, 1 = left
     uint1 diy;  //0 = down, 1 = up

     uint8 cr;
     uint9 match;
     uint1 found;
     uint1 eq;
     uint1 maj;

     uint4 logic;

    //internal latches
      uint3 lgm;
      uint9 lsx;
      uint9 ldx;
      uint9 lnx;
  } op;
};

}
