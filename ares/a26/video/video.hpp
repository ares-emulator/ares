struct Video {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean phosphor;

  enum class Sync : u32 { Waiting, Pending, Qualified };

  auto displayHeight() const -> u32 { return Region::NTSC() ? 228 : 243; }
  auto verticalOffset() const -> s32 { return Region::NTSC() ? 20 : 40; }

  auto line() const -> u32 { return lineCounter; }
  auto y() const -> i32 { return (i32)lineCounter - verticalOffset(); }

  //video.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;
  auto setPhosphor(bool) -> void;

  //receiver.cpp
  auto clock(i16 x, n7 pixel, n1 hblank, n1 vblank) -> void;
  auto fill(i16 x, n7 pixel) -> void;
  auto endScanline() -> void;
  auto vsync(n1 level) -> void;
  auto frame() -> void;
  auto accept() -> void;
  auto fallback() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  Sync sync;
  u32 lineCounter;
  u32 linesSinceReturn;
  u8 pulseLines;
};

extern Video video;
