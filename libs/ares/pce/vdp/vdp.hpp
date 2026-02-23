#include "vce.hpp"
#include "vdc.hpp"
#include "vpc.hpp"

struct VDPBase {
  struct Implementation : Thread {
    struct IO {
      n16 hcounter;
      n16 vcounter;
    } io;

    virtual auto irqLine() const -> bool = 0;
    virtual auto load(Node::Object) -> void = 0;
    virtual auto unload() -> void = 0;
    virtual auto power() -> void = 0;
    virtual auto serialize(serializer&) -> void = 0;
  };

  Implementation* implementation = nullptr;
  VCEBase* vce = nullptr;
  VDCBase* vdc0 = nullptr;
  VDCBase* vdc1 = nullptr;
  VPCBase* vpc = nullptr;
  bool accurate = false;

  auto setAccurate(bool value) -> void;

  auto thread() const -> Thread& { return *implementation; }
  auto hcounter() const -> n16 { return implementation->io.hcounter; }
  auto vcounter() const -> n16 { return implementation->io.vcounter; }

  auto irqLine() const -> bool { return implementation->irqLine(); }
  auto load(Node::Object parent) -> void { implementation->load(parent); }
  auto unload() -> void { implementation->unload(); }
  auto power() -> void { implementation->power(); }
  auto serialize(serializer& s) -> void { implementation->serialize(s); }
};

extern VDPBase vdp;

struct VDP : VDPBase::Implementation {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Setting::Boolean colorEmulation;

  auto irqLine() const -> bool override { return vdc0.irqLine() | vdc1.irqLine(); }

  //vdp.cpp
  auto load(Node::Object) -> void override;
  auto unload() -> void override;

  template<bool supergrafx> auto main() -> void;
  template<bool supergrafx> auto step(u32 clocks) -> void;
  auto power() -> void override;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void override;

  VCE vce;
  VDC vdc0;
  VDC vdc1;
  VPC vpc;
};

extern VDP vdpImpl;
