namespace Memory {
  #include "interface.hpp"
  #include "readable.hpp"
  #include "writable.hpp"
  #include "unmapped.hpp"
}

//System Bus
struct Bus {
  //bus.hpp
  auto mmio(u32 address) -> Memory::Interface&;
  template<uint Size> auto read(u32 address) -> u32;
  template<uint Size> auto write(u32 address, u32 data) -> void;
};

struct MemoryControl : Memory::Interface {
  Node::Object node;

  //memory.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct RAM {
    uint32 value;
     uint1 delay;   //1 = add one cycle on simultaneous code+data fetches
     uint3 window;  //size and mirroring/access control
  } ram;

  struct Cache {
     uint1 lock;               //not emulated
     uint1 invalidate;         //not emulated
     uint1 tagTest;
     uint1 scratchpadEnable;
     uint2 dataSize;           //not emulated
     uint1 dataEnable;
     uint2 codeSize;           //not emulated
     uint1 codeEnable;
     uint1 interruptPolarity;  //not emulated
     uint1 readPriority;       //not emulated
     uint1 noWaitState;        //not emulated
     uint1 busGrant;           //not emulated
     uint1 loadScheduling;     //not emulated
     uint1 noStreaming;        //not emulated
    uint14 reserved;           //not used
  } cache;
};

struct MemoryExpansion : Memory::Interface {
  MemoryExpansion(uint byte, uint half, uint word) {
    Memory::Interface::setWaitStates(byte, half, word);
  }

  auto readByte(u32 address) -> u32 { return 0; }
  auto readHalf(u32 address) -> u32 { return 0; }
  auto readWord(u32 address) -> u32 { return 0; }
  auto writeByte(u32 address, u32 data) -> void { return; }
  auto writeHalf(u32 address, u32 data) -> void { return; }
  auto writeWord(u32 address, u32 data) -> void { return; }
};

extern Bus bus;
extern MemoryControl memory;
extern MemoryExpansion expansion1;
extern MemoryExpansion expansion2;
extern MemoryExpansion expansion3;
extern Memory::Readable bios;
extern Memory::Unmapped unmapped;
