#define VCE VCEPerformance
#define VDC VDCPerformance
#define VDP VDPPerformance
#define VPC VPCPerformance

#include "vce.hpp"
#include "vdc.hpp"
#include "vpc.hpp"

struct VDP : VDPBase::Implementation {
  Node::Object node;
  Node::Video::Screen screen;

  auto irqLine() const -> bool override { return vdc0.irqLine() | vdc1.irqLine(); }

  //vdp.cpp
  auto load(Node::Object) -> void override;
  auto unload() -> void override;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void override;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void override;

  VCE vce;
  VDC vdc0;
  VDC vdc1;
  VPC vpc;

  n32 renderingCycle;
};

extern VDP vdpPerformanceImpl;

#undef VCE
#undef VDC
#undef VDP
#undef VPC
