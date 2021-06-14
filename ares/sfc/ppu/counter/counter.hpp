//PPUcounter emulates the H/V latch counters of the S-PPU2.
//
//real hardware has the S-CPU maintain its own copy of these counters that are
//updated based on the state of the S-PPU Vblank and Hblank pins. emulating this
//would require full lock-step synchronization for every clock tick.
//to bypass this and allow the two to run out-of-order, both the CPU and PPU
//classes inherit PPUcounter and keep their own counters.
//the timers are kept in sync, as the only differences occur on V=240 and V=261,
//based on interlace. thus, we need only synchronize and fetch interlace at any
//point before this in the frame, which is handled internally by this class at
//V=128.

struct PPUcounter {
  //inline.hpp
  auto tick() -> void;
  auto tick(u32 clocks) -> void; private:
  auto tickScanline() -> void; public:

  auto interlace() const -> bool;
  auto field() const -> bool;
  auto vcounter() const -> u32;
  auto hcounter() const -> u32;
  auto hdot() const -> u32; private:
  auto vperiod() const -> u32; public:
  auto hperiod() const -> u32;

  auto vcounter(u32 offset) const -> u32;
  auto hcounter(u32 offset) const -> u32;

  auto reset() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  function<void ()> scanline;

private:
  struct {
    bool interlace;
    bool field;
    u32  vperiod;
    u32  hperiod;
    u32  vcounter;
    u32  hcounter;
  } time;

  struct {
    u32 vperiod;
    u32 hperiod;
  } last;
};
