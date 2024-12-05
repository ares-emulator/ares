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
    auto ioMem(bool mode, u32 address, u32 data, const char *name) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory ram;
      Node::Debugger::Memory scratch;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification interrupt;
      Node::Debugger::Tracer::Notification io;
      Node::Debugger::Tracer::Notification ioMem;
    } tracer;
  } debugger;

  inline auto secure() const -> bool {
    return bb_exc.secure;
  }

  //mi.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  enum class IRQ : u32 { SP, SI, AI, VI, PI, DP, FLASH, AES, IDE, PI_ERR, USB0, USB1, BTN, MD };
  auto raise(IRQ) -> void;
  auto lower(IRQ) -> void;
  auto poll() -> void;
  auto enter_secure_mode() const -> bool;

  auto power(bool reset) -> void;

  //io.cpp
  auto ioRead(u32 address) -> u32;
  auto ioWrite(u32 address, u32 data_) -> void;
  auto readWord(u32 address_, Thread& thread) -> u32;
  auto writeWord(u32 address_, u32 data, Thread& thread) -> void;

  template<u32 Size>
  inline auto writeBurst(u32 address, u32 *value, Thread& thread, const char *peripheral) -> void {
    writeWord(address | 0x00, value[0], thread);
    writeWord(address | 0x04, value[1], thread);
    writeWord(address | 0x08, value[2], thread);
    writeWord(address | 0x0c, value[3], thread);
    if (Size == ICache) {
      writeWord(address | 0x10, value[4], thread);
      writeWord(address | 0x14, value[5], thread);
      writeWord(address | 0x18, value[6], thread);
      writeWord(address | 0x1c, value[7], thread);
    }
  }

  template<u32 Size>
  inline auto readBurst(u32 address, u32 *value, Thread& thread, const char *peripheral) -> void {
    value[0] = readWord(address | 0x00, thread);
    value[1] = readWord(address | 0x04, thread);
    value[2] = readWord(address | 0x08, thread);
    value[3] = readWord(address | 0x0c, thread);
    if (Size == ICache) {
      value[4] = readWord(address | 0x10, thread);
      value[5] = readWord(address | 0x14, thread);
      value[6] = readWord(address | 0x18, thread);
      value[7] = readWord(address | 0x1c, thread);
    }
  }

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Interrupt {
    b1 line;
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
    u8 io  = 0x02;  //I/O interface
    u8 rac = 0x01;  //RAMBUS ASIC cell
    static constexpr u8 rdp = 0x02;  //Reality Display Processor
    static constexpr u8 rsp = 0x02;  //Reality Signal Processor
  } revision;

  struct BB {
    n1 button;
    n1 card;
  } bb;

  struct BBTrapCause {
    n1 application;
    n1 timer;
    n1 pi_error;
    n1 mi_error;
    n1 button;
    n1 md;
  } bb_trap;

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
