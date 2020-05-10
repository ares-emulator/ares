//Serial Interface

struct SI {
  //si.cpp
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    uint24 dramAddress = 0;
    uint32 readAddress = 0;
    uint32 writeAddress = 0;
     uint1 dmaBusy = 0;
     uint1 ioBusy = 0;
     uint1 readPending = 0;
     uint4 pchState = 0;
     uint4 dmaState = 0;
     uint1 dmaError = 0;
     uint1 interrupt = 0;
  } io;
};

extern SI si;
