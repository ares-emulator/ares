//Peripheral Interface

struct PI {
  Memory rom;
  Memory ram;

  //pi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
     uint1 dmaBusy = 0;
     uint1 ioBusy = 0;
     uint1 error = 0;
     uint1 interrupt = 0;
    uint32 dramAddress = 0;
    uint32 pbusAddress = 0;
    uint32 readLength = 0;
    uint32 writeLength = 0;
  } io;
};

extern PI pi;
