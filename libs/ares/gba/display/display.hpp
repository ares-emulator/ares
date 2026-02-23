struct Display : Thread, IO {
  Node::Object node;

  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void;
  auto main() -> void;

  auto power() -> void;

  //io.cpp
  auto readIO(n32 address) -> n8;
  auto writeIO(n32 address, n8 byte) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    n1  vblank;
    n1  hblank;
    n1  vcoincidence;
    n1  irqvblank;
    n1  irqhblank;
    n1  irqvcoincidence;
    n8  vcompare;

    n16 vcounter;
  } io;
  
  n1 videoCapture = 0;
};

extern Display display;
