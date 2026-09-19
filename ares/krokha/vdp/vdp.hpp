//there is no video chip: a counter chain scans the 2KB of system RAM as a
//64x32 character grid and looks each byte up in the cartridge's character ROM.
//only the rightmost 48 columns leave the tube, so columns 0-15 are free for
//the cartridge program to use as scratch and stack space

struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto scanline() -> void;
  auto frame() -> void;

  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  static constexpr u32 dotsPerLine   = 512;
  static constexpr u32 linesPerFrame = 312;
  static constexpr u32 width         = 48 * 8;
  static constexpr u32 height        = 32 * 8;

private:
  u32 vcounter = 0;
};

extern VDP vdp;
