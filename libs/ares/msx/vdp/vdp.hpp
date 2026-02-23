struct VDP : TMS9918, V9938, Thread {
  Node::Object node;
  Node::Video::Screen screen;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void override;
  auto irq(bool line) -> void override;
  auto frame() -> void override;
  auto power() -> void;

  auto read(n2 port) -> n8;
  auto write(n2 port, n8 data) -> void;

  //color.cpp
  auto colorMSX(n32) -> n64;
  auto colorMSX2(n32) -> n64;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern VDP vdp;
