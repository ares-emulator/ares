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
  template<bool isWrite, bool isDMA> auto calcAccessTime(u32 address, u32 bytesCount = 0) -> u32 const;
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

  struct MemPort {
    n4  writeDelay = 0xf; // 0..F = 1..16 cycles
    n4  readDelay = 0xf;  // 0..F = 1..16 cycles
    n1  recovery;         // uses COM0
    n1  hold;             // uses COM1
    n1  floating;         // uses COM2
    n1  preStrobe;        // uses COM3
    n1  dataWidth;        // 0=8-bit, 1=16-bit
    n1  autoIncrement;    // auto-increment address on multi-word ops
    n2  unknown14_15;
    n5  addrBits;         // number of address bits (window size = 1<<N)
    n3  reserved21_23;    // always zero
    n4  dmaTiming;        // DMA override cycles
    n1  addrError;        // write 1 to clear
    n1  dmaSelect;        // 0=normal, 1=use dmaTiming
    n1  wideDMA;          // 0=use dataWidth, 1=force 32-bit
    n1  wait;             // wait for external device


    template<bool isWrite, bool isDMA> auto calcAccessTime(u32 bytesCount = 0) -> u32 const;
  };

  // Global COM delay register
  struct CommonDelay {
    n4 com0;  // recovery
    n4 com1;  // hold
    n4 com2;  // floating
    n4 com3;  // pre-strobe
    n16 unused;
  } common;

  MemPort exp1;
  MemPort exp2;
  MemPort exp3;
  MemPort bios;
  MemPort spu;
  MemPort cdrom;
};

struct MemoryExpansion : Memory::Interface {
  auto readByte(u32 address) -> u32 { return 0xff; }
  auto readHalf(u32 address) -> u32 { return 0xffff; }
  auto readWord(u32 address) -> u32 { return 0xffff'ffff; }
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
