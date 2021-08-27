#pragma once

namespace ares {

//Texas Instruments TMS9918 family

struct TMS9918 {
  Node::Video::Screen screen;
  Memory::Writable<n8> vram;

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto irq(bool line) -> void = 0;
  virtual auto frame() -> void = 0;

  auto videoMode() const -> n4 { return io.videoMode; }
  auto vcounter() const -> n8 { return io.vcounter; }
  auto hcounter() const -> n8 { return io.hcounter; }

  //tms9918.cpp
  auto load(Node::Video::Screen) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto poll() -> void;
  auto power() -> void;

  //io.cpp
  auto status() -> n8;
  auto data() -> n8;

  auto data(n8) -> void;
  auto control(n8) -> void;
  auto register(n3, n8) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  struct Background {
    TMS9918& self;

    //background.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto text1(n8 hoffset, n8 voffset) -> void;
    auto graphics1(n8 hoffset, n8 voffset) -> void;
    auto graphics2(n8 hoffset, n8 voffset) -> void;
    auto multicolor(n8 hoffset, n8 voffset) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n4 nameTableAddress;
      n8 colorTableAddress;
      n3 patternTableAddress;
    } io;

    struct Output {
      n4 color;
    } output;
  } background{*this};

  struct Sprite {
    TMS9918& self;

    //sprite.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Object {
      i9 x;
      i9 y = 0xd0;
      n8 pattern;
      n4 color;
    } objects[4];

    struct IO {
      n1 zoom;
      n1 size;
      n7 attributeTableAddress;
      n3 patternTableAddress;

      //flags
      n5 overflowIndex = 0b11111;
      n1 overflow;
      n1 collision;
    } io;

    struct Output {
      n4 color;
    } output;
  } sprite{*this};

  struct DAC {
    TMS9918& self;

    //dac.cpp
    auto setup(n8 voffset) -> void;
    auto run(n8 hoffset, n8 voffset) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 displayEnable;
      n1 externalSync;  //todo
      n4 colorBackground;
      n4 colorForeground;  //todo
    } io;

  //unserialized:
    u32* output = nullptr;
  } dac{*this};

  struct IRQFrame {
    n1 enable;
    n1 pending;
  } irqFrame;

  struct IO {
    n9  vcounter;
    n8  hcounter;

    n1  controlLatch;
    n16 controlValue;
    n8  vramLatch;

    n3  videoMode;
    n1  vramMode = 1;  //0 = 4KB; 1 = 16KB
  } io;
};

}
