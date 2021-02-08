#if defined(PROFILE_PERFORMANCE)
#include "../vdp-performance/vdp.hpp"
#else
#include "vce.hpp"
#include "vdc.hpp"
#include "vpc.hpp"

struct VDP : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean overscan;

  auto irqLine() const -> bool { return vdc0.irqLine() | vdc1.irqLine(); }

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  VCE vce;
  VDC vdc0;
  VDC vdc1;
  VPC vpc;

  struct IO {
    n16 hcounter;
    n16 vcounter;
  } io;
};

extern VDP vdp;
#endif
