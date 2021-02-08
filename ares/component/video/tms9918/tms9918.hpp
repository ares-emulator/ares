#pragma once

namespace ares {

//Texas Instruments TMS9918 family

struct TMS9918 {
  Node::Video::Screen screen;

  virtual auto step(u32 clocks) -> void = 0;
  virtual auto irq(bool line) -> void = 0;
  virtual auto frame() -> void = 0;

  //tms9918.cpp
  auto load(Node::Video::Screen) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto power() -> void;

  //io.cpp
  auto status() -> n8;
  auto data() -> n8;

  auto data(n8) -> void;
  auto control(n8) -> void;
  auto register(n3, n8) -> void;

  //background.cpp
  auto background(n8 hoffset, n8 voffset) -> void;
  auto text1(n8 hoffset, n8 voffset) -> void;
  auto graphics1(n8 hoffset, n8 voffset) -> void;
  auto graphics2(n8 hoffset, n8 voffset) -> void;
  auto multicolor(n8 hoffset, n8 voffset) -> void;

  //sprites.cpp
  auto sprite(n8 voffset) -> void;
  auto sprite(n8 hoffset, n8 voffset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  Memory::Writable<n8> vram;

  struct IO {
    u32 vcounter = 0;
    u32 hcounter = 0;

    n1  controlLatch;
    n16 controlValue;
    n8  vramLatch;

    n5  spriteOverflowIndex;
    n1  spriteCollision;
    n1  spriteOverflow;
    n1  irqLine;

    n1  externalInput;
    n3  videoMode;
    n1  spriteZoom;
    n1  spriteSize;
    n1  irqEnable;
    n1  displayEnable;
    n1  ramMode = 1;  //0 = 4KB; 1 = 16KB
    n4  nameTableAddress;
    n8  colorTableAddress;
    n3  patternTableAddress;
    n7  spriteAttributeTableAddress;
    n3  spritePatternTableAddress;
    n4  colorBackground;
    n4  colorForeground;
  } io;

  struct Sprite {
    n8  x;
    n8  y;
    n8  pattern;
    n4  color;
  } sprites[4];

  struct Output {
    n4  color;
  } output;
};

}
