//Reality Display Processor

struct RDP : Thread {
  Node::Component node;

  //rdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //io.cpp
  auto readIO(u32 address) -> u32;
  auto writeIO(u32 address, u32 data) -> void;

  //io-scc.cpp
  auto readSCC(u32 address) -> u32;
  auto writeSCC(u32 address, uint32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Command {
     uint1 source = 0;  //0 = RDRAM, 1 = DMEM
    uint24 start = 0;
    uint24 end = 0;
    uint24 current = 0;
    uint24 clock = 0;
    uint24 bufferBusy = 0;
    uint24 pipeBusy = 0;
    uint24 tmemBusy = 0;
  } command;
};

extern RDP rdp;
