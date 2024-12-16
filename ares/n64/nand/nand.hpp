//NAND

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

  Node::Object node;

  Memory::Writable data = {};
  Memory::Writable spare = {};
  Memory::Writable writeBuffer = {};
  Memory::Writable writeBufferSpare = {};

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto command(Command cmd, string desc) -> void;

    struct Memory {
      Node::Debugger::Memory nand;
      Node::Debugger::Memory spare;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;

    u32 num;
  } debugger;

  NAND(u32 n) {
    debugger.num = n;
  }

  //nand.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;
  auto read(Memory::Writable& dest, b1 which, n27 pageNum, n10 length) -> void;
  auto readId(Memory::Writable& dest, b1 which, n10 length) -> void;
  auto writeToBuffer(Memory::Writable& src, b1 which, n27 pageNum, n10 length) -> void;
  auto commitWriteBuffer(n27 pageNum) -> void;
  auto readStatus(Memory::Writable& dest, b1 which, n10 length, bool multiplane) -> void;
  auto queueErasure(n27 pageNum) -> void;
  auto execErasure() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n10 pageOffset = 0;
  n4 eraseQueueOccupied = 0;
  n27 eraseQueuePage[4] = {};
};

extern NAND nand0;
extern NAND nand1;
extern NAND nand2;
extern NAND nand3;
