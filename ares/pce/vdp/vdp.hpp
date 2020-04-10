#if defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.hpp"
#else
#include "vce.hpp"
#include "vdc.hpp"
#include "vpc.hpp"

struct VDP : Thread {
  Node::Component node;
  Node::Screen screen;
  Node::Boolean overscan;

  auto irqLine() const -> bool { return vdc0.irqLine() | vdc1.irqLine(); }

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;
  auto power() -> void;

  //color.cpp
  auto color(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint32 buffer[1365 * 263];

  VCE vce;
  VDC vdc0;
  VDC vdc1;
  VPC vpc;

  struct IO {
    uint16 hcounter;
    uint16 vcounter;
  } io;
};

extern VDP vdp;
#endif
