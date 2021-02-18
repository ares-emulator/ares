struct PeripheralDevice {
  Node::Peripheral node;

  virtual ~PeripheralDevice() = default;
  virtual auto reset() -> void {}
  virtual auto acknowledge() -> bool { return 0; }
  virtual auto bus(u8 data) -> u8 { return 0xff; }
};

struct Peripheral : Thread, Memory::Interface {
  Node::Object node;

  //peripheral.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto receive() -> u8;
  auto transmit(u8 data) -> void;
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IO {
    //JOY_RX_DATA
    n64 receiveData;
    n8  receiveSize;

    //JOY_TX_DATA
    n8 transmitData;

    //JOY_STAT
    n1 transmitStarted = 1;
    n1 transmitFinished = 1;
    n1 parityError;
    n1 interruptRequest;

    //JOY_MODE
    n2 baudrateReloadFactor;
    n2 characterLength;
    n1 parityEnable;
    n1 parityType;
    n2 unknownMode_6_7;
    n1 clockOutputPolarity;
    n7 unknownMode_9_15;

    //JOY_CTRL
    n1 transmitEnable;
    n1 joyOutput;
    n1 receiveEnable;
    n1 unknownCtrl_3;
    n1 acknowledge;
    n1 unknownCtrl_5;
    n1 reset;
    n1 unknownCtrl_7;
    n2 receiveInterruptMode;
    n1 transmitInterruptEnable;
    n1 receiveInterruptEnable;
    n1 acknowledgeInterruptEnable;
    n1 slotNumber;
    n2 unknownCtrl_14_15;

    //JOY_BAUD
    n16 baudrateReloadValue;

    //internal
    i32 counter;
  } io;
};

#include "port.hpp"
#include "digital-gamepad/digital-gamepad.hpp"
#include "memory-card/memory-card.hpp"
extern Peripheral peripheral;
