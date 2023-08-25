struct TIA : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Audio::Stream stream;

  auto vlines() const -> u32 { return Region::NTSC() ? 262 : 312; }
  auto displayHeight() const -> u32 { return Region::NTSC() ? 192 : 228; }

  //tia.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks = 1) -> void;
  auto power(bool reset) -> void;
  auto frame() -> void;
  auto scanline() -> void;
  auto pixel(n8 x) -> void;

  auto runPlayfield(n8 x) -> n1;
  auto runBall(n8 x) -> n1;
  auto runPlayer(n8 x, n1 index) -> n1;
  auto runMissile(n8 x, n1 index) -> n1;
  auto runCollision() -> void;

  //io.cpp
  auto read(n8 address) -> n8;
  auto write(n8 address, n8 data) -> void;
  auto vsync(n1 state) -> void;
  auto ctrlpf(n8 data) -> void;
  auto cxclr() -> void;
  auto hmove() -> void;
  auto hmclr() -> void;
  auto grp(n1 index, n8 data) -> void;
  auto resp(n1 index) -> void;
  auto resm(n1 index) -> void;
  auto resmp(n1 index, n8 data) -> void;
  auto resbl() -> void;

  // audio.cpp
  auto runAudio() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  // write-queue.cpp
  struct WriteQueue {
    inline auto add(u8 address, n8 data, i8 delay) -> void;
    inline auto step() -> void;

    struct QueueItem {
      inline auto commit() -> void;
      n1 active;
      i8 address;
      n8 data;
      i8 delay;
    };

    QueueItem items[16];
    const int maxItems = 16;
  } writeQueue;

  struct {
    u9 vcounter;
    u8 hcounter;
    u8 hmoveTriggered;

    n1 vsync;
    n1 vblank;

    n7 bgColor;
    n7 p0Color;
    n7 p1Color;
    n7 fgColor;
  } io;

  struct {
    n20 graphics;
    n1 pixel;
    n1 mirror;
    n1 scoreMode;
    n1 priority;
  } playfield;

  struct {
    n8 graphics[2];
    n1 reflect;
    n3 size;
    i4 offset;
    n8 position;
    n1 delay;
  } player[2];

  struct {
    n1 enable;
    n1 reset;
    n2 size;
    i4 offset;
    n8 position;
  } missile[2];

  struct {
    n1 enable[2];
    n2 size;
    i4 offset;
    n1 delay;
    n8 position;
  } ball;

  struct {
    n1 M0P0;
    n1 M0P1;
    n1 M1P0;
    n1 M1P1;
    n1 P0PF;
    n1 P0BL;
    n1 P1PF;
    n1 P1BL;
    n1 M0PF;
    n1 M0BL;
    n1 M1PF;
    n1 M1BL;
    n1 BLPF;
    n1 P0P1;
    n1 M0M1;
  } collision;

  struct AudioChannel {
    auto phase0() -> void;
    auto phase1() -> u8;
    n1 enable;
    n8 divCounter;
    n8 noiseCounter;
    n1 noiseFeedback;
    n8 pulseCounter;
    n1 pulseCounterPaused;
    n1 pulseFeedback;
    n4 volume;
    n4 control;
    n5 frequency;
  } audio[2];

  f64 volume[16];
};

extern TIA tia;