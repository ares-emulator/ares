// Virage Non-Volatile Flash Memory

struct Virage : Memory::RCP<Virage> {
  Node::Object node;

  Memory::Writable sram;
  Memory::Writable flash;

  struct Debugger {
    Virage& self;

    Debugger(Virage& self) : self(self) {}

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    auto ioSRAM(bool mode, u32 address, u32 data) -> void;
    auto ioConfigReg(bool mode, u32 address, u32 data) -> void;
    auto ioStatusReg(bool mode, u32 data) -> void;
    auto ioControlReg(bool mode, u32 data) -> void;

    struct Memory {
      Node::Debugger::Memory sram;
      Node::Debugger::Memory flash;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
      Node::Debugger::Tracer::Notification ioMem;
    } tracer;
  } debugger{*this};

  Virage(u32 n, u32 size) {
    num = n;
    memSize = size;

    queueID = (n == 0) ? Queue::VIRAGE0_Command :
              (n == 1) ? Queue::VIRAGE1_Command :
              Queue::VIRAGE2_Command;
  }

  //virage.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto commandFinished() -> void;
  auto command(u32 command) -> void;
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  u32 num;
  u32 queueID;
  u32 memSize;

  struct IO {
    n32 configReg[6];

    n1 busy;
    n1 loadDone;
    n1 storeDone;
    n1 unk30;

    n32 command;
  } io;
};

extern Virage virage0;
extern Virage virage1;
extern Virage virage2;
