//NAND

#define MEM_ECC 1
#include "ecc.hpp"

struct NAND : Memory::RCP<NAND> {
  enum Command : u8 {
    Read0                  = 0x00,
    Read1                  = 0x01,
    ReadSpare              = 0x50,
    ReadID                 = 0x90,
    Reset                  = 0xFF,
    PageProgramC1          = 0x80,
    PageProgramC2          = 0x10,
    PageProgramDummyC2     = 0x11,
    CopyBackProgramC2      = 0x8A,
    CopyBackProgramDummyC1 = 0x03,
    BlockEraseC1           = 0x60,
    BlockEraseC2           = 0xD0,
    ReadStatus             = 0x70,
    ReadStatusMultiplane   = 0x71,
  };
  static constexpr u8 NUM_PLANES = 4;

  Node::Object node;

  struct Debugger {
    NAND& self;

    Debugger(NAND& self) : self(self) {}

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object parent) -> void;
    auto command(Command cmd, string desc) -> void;

    struct Memory {
      Node::Debugger::Memory nand;
      Node::Debugger::Memory spare;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;

  } debugger{*this};

  NAND(u32 n) : num(n) {}

  inline auto setPageOffset(n10 newPageOffset) {
    pageOffset = newPageOffset;
  }

  //nand.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;
  auto read(Memory::Writable& dest, b1 which, n27 pageNum, n10 length, b1 ecc) -> n2;
  auto readId(Memory::Writable& dest, b1 which, n10 length) -> void;
  auto writeToBuffer(Memory::Writable& src, b1 which, n27 pageNum, n10 length) -> void;
  auto queueWriteBuffer(n27 pageNum, bool commit = false) -> void;
  auto commitWriteBuffer(n27 pageNum) -> void;
  auto readStatus(Memory::Writable& dest, b1 which, n10 length, bool multiplane) -> void;
  auto queueErasure(n27 pageNum) -> void;
  auto execErasure() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  u32 num;
  n10 pageOffset = 0;

  Memory::Writable data = {};
  Memory::Writable spare = {};
  Memory::Writable writeBuffers[NUM_PLANES] = {};
  Memory::Writable writeBufferSpares[NUM_PLANES] = {};

  n4 eraseQueueOccupied = 0;
  n27 eraseQueueAddrs[NUM_PLANES] = {};
  n4 writeBuffersOccupied = 0;
  n27 writeBufferAddrs[NUM_PLANES] = {};
};

extern NAND nand0;
extern NAND nand1;
extern NAND nand2;
extern NAND nand3;
