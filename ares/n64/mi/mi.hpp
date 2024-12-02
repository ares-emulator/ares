//MIPS Interface

struct MI : Memory::RCP<MI> {
  Node::Object node;
  Memory::Readable rom;
  Memory::Writable ram;
  Memory::Writable scratch;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto interrupt(u8 source) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification interrupt;
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //mi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  enum class IRQ : u32 { SP, SI, AI, VI, PI, DP, FLASH, AES, IDE, PI_ERR, USB0, USB1, BTN, MD };
  auto raise(IRQ) -> void;
  auto lower(IRQ) -> void;
  auto poll() -> void;

  auto power(bool reset) -> void;

  //io.cpp
  auto ioRead(u32 address) -> u32;
  auto ioWrite(u32 address, u32 data_) -> void;
  auto readWord(u32 address_, Thread& thread) -> u32;
  auto writeWord(u32 address_, u32 data, Thread& thread) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Interrupt {
    b1 line = 1;
    b1 mask;
  };

  struct IRQs {
    Interrupt sp;
    Interrupt si;
    Interrupt ai;
    Interrupt vi;
    Interrupt pi;
    Interrupt dp;
  } irq;

  struct BBIRQs {
    Interrupt flash;
    Interrupt aes;
    Interrupt ide;
    Interrupt pi_err;
    Interrupt usb0;
    Interrupt usb1;
    Interrupt btn;
    Interrupt md;
  } bb_irq;

  struct IO {
    n7 initializeLength;
    n1 initializeMode;
    n1 ebusTestMode;
    n1 rdramRegisterSelect;
  } io;

  struct Revision {
    static constexpr u8 io  = 0x02;  //I/O interface
    static constexpr u8 rac = 0x01;  //RAMBUS ASIC cell
    static constexpr u8 rdp = 0x02;  //Reality Display Processor
    static constexpr u8 rsp = 0x02;  //Reality Signal Processor
  } revision;

  struct BB {
    n1 button;
    n1 card;
  } bb;

  struct BBException {
    n1 secure;
    n1 boot_swap = 1;
    n1 application;
    n1 timer;
    n1 pi_error;
    n1 mi_error;
    n1 button;
    n1 md;
    n1 sk_ram_access;
  } bb_exc;
};

extern MI mi;
