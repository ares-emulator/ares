//the LCD controller is a Bit Corp ASIC with no part number of its own. it scans
//16KB of private VRAM as two interleaved bitplanes giving four shades, and the
//CPU reaches that VRAM only through an auto-incrementing port.
//
//documented by Kevtris: http://blog.kevtris.org/blogfiles/Gamate%20Inside.txt

struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;

  //2x 8KB SRAM, interleaved a plane at a time
  Memory::Writable<n8> vram;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto read(n3 address) -> n8;
  auto write(n3 address, n8 data) -> void;

  auto scanline() -> void;
  auto frame() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  static constexpr u32 width  = 160;
  static constexpr u32 height = 150;

  //72900 master clocks a frame across 150 lines, so 243 CPU cycles a line and
  //no vertical blanking period at all
  static constexpr u32 cyclesPerLine = 243;

private:
  auto vramAddress() const -> n16;
  auto increment() -> void;
  auto origin(u32 line, u32& x, u32& y) -> void;
  auto pixel(u32 x, u32 y) -> n2;

  struct Registers {
    n13 address;      //word address into VRAM; the byte address is twice this
    n1  plane;        //which of the two bitplanes the port reads and writes
    n8  scrollX;
    n8  scrollY;
    n1  blank;        //blank the panel without stopping the refresh
    n1  incrementRow; //advance by a row rather than a byte
    n1  window;       //lock the top 16 lines to VRAM rows d0-df
    n1  swapPlanes;
  } io;

  u32 vcounter = 0;
};

extern VDP vdp;
