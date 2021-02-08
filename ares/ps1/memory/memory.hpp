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
  template<u32 Size> auto read(u32 address) -> u32;
  template<u32 Size> auto write(u32 address, u32 data) -> void;
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
    n32 value;
    n1  delay;   //1 = add one cycle on simultaneous code+data fetches
    n3  window;  //size and mirroring/access control
  } ram;

  struct Cache {
    n1  lock;               //not emulated
    n1  invalidate;         //not emulated
    n1  tagTest;
    n1  scratchpadEnable;
    n2  dataSize;           //not emulated
    n1  dataEnable;
    n2  codeSize;           //not emulated
    n1  codeEnable;
    n1  interruptPolarity;  //not emulated
    n1  readPriority;       //not emulated
    n1  noWaitState;        //not emulated
    n1  busGrant;           //not emulated
    n1  loadScheduling;     //not emulated
    n1  noStreaming;        //not emulated
    n14 reserved;           //not used
  } cache;
};

struct MemoryExpansion : Memory::Interface {
  MemoryExpansion(u32 byte, u32 half, u32 word) {
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
