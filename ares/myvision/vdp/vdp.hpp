struct VDP : TMS9918, Thread {
  Node::Object node;
  Node::Video::Screen screen;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void override;
  auto irq(bool line) -> void override;
  auto frame() -> void override;
  auto power() -> void;

  //color.cpp
  auto color(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern VDP vdp;
