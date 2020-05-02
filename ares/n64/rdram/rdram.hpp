struct RDRAM {
  Memory ram;

  //rdram.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint6 currentControlInput;
    uint1 currentControlEnable;
  } io;
};

extern RDRAM rdram;
