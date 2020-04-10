struct VDP : TMS9918, V9938, Thread {
  Node::Component node;
  Node::Screen screen_;

  //vdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(uint clocks) -> void override;
  auto irq(bool line) -> void override;
  auto frame() -> void override;
  auto refresh() -> void;
  auto power() -> void;

  auto read(uint2 port) -> uint8;
  auto write(uint2 port, uint8 data) -> void;

  //color.cpp
  auto colorMSX(uint32) -> uint64;
  auto colorMSX2(uint32) -> uint64;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern VDP vdp;
