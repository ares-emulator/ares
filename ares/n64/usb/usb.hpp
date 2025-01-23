// USB Controller

struct USB : Memory::RCP<USB> {
  Node::Object node;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 address, u32 data) -> void;

    auto ioSRAM(bool mode, u32 address, u32 data) -> void;
    auto ioConfigReg(bool mode, u32 address, u32 data) -> void;
    auto ioStatusReg(bool mode, u32 data) -> void;
    auto ioControlReg(bool mode, u32 data) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  inline auto num() -> u32 {
    return n;
  }

  USB(u32 n) : n(n) {}

  //usb.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto ioRead(u32 address, Thread& thread) -> u32;
  auto ioWrite(u32 address, u32 data, Thread& thread) -> void;
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  u32 n;
  Memory::Writable bdt;
  
  struct IO {
    static constexpr u8 perid = 0x04;
    static constexpr u8 idcomp = 0xFB;
    static constexpr u8 rev = 0x30;
    static constexpr u8 addinfo = 0x01;

    struct {
      n1 id;
      n1 oneMSec;
      n1 lineState;
      n1 sessionValid;
      n1 bSession;
      n1 aVBus;
    } otgIntStatus;

    struct {
      n1 id;
      n1 oneMSec;
      n1 lineState;
      n1 sessionValid;
      n1 bSession;
      n1 aVBus;
    } otgIntEnable;

    struct {
      n1 id;
      n1 oneMSec;
      n1 lineStateStable;
      n1 sessionValid;
      n1 bSessionEnd;
      n1 aVbusValid;
    } otgStatus;

    struct {
      n1 dpHigh;
      n1 dpLow;
      n1 dmLow;
      n1 otgEnable;
    } otgControl;

    n1 access = 0;
  } io;
};

extern USB usb0;
extern USB usb1;
