struct PeripheralDevice {
  Node::Peripheral node;

  virtual ~PeripheralDevice() = default;
  virtual auto save() -> void {}
  virtual auto reset() -> void {}
  virtual auto acknowledge() -> bool { return 0; }
  virtual auto active() -> bool { return 0; }
  virtual auto bus(u8 data) -> u8 { return 0xff; }
};

struct Peripheral : Memory::Interface {
  Node::Object node;

  //peripheral.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto nextEvent() const -> u32;
  auto baudrateReload() const -> u32;
  auto stepBaudrateTimer(u32 clocks) -> u32;

  static constexpr s32 controllerAckDelay = 338;
  static constexpr s32 memoryCardAckDelay = 1500;
  static constexpr s32 ackPulseWidth = 96;
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

    //JOY_STAT
    n1 transmitStarted;
    n1 transmitFinished;
    n1 acknowledgeAsserted;
    n1 parityError;
    n1 interruptRequest;

    //JOY_MODE
    n2 baudrateReloadFactor;
    n2 characterLength;
    n1 parityEnable;
    n1 parityType;
    n1 clockOutputPolarity;

    //JOY_CTRL
    n1 transmitEnable;
    n1 joyOutput;
    n1 receiveEnable;
    n1 unknownCtrl_3;
    n1 unknownCtrl_5;
    n2 receiveInterruptMode;
    n1 transmitInterruptEnable;
    n1 receiveInterruptEnable;
    n1 acknowledgeInterruptEnable;
    n1 slotNumber;

    //JOY_BAUD
    n16 baudrateReloadValue;

    n16 sioMode;
    n16 sioControl;
    n16 sioInternal;
    n16 sioBaudrateReloadValue;

    //internal
    u32 baudrateTimer;
    u32 transferElapses;
    i32 ackDelayCounter;
    i32 ackAssertCounter;
  } io;
};

#include "port.hpp"
#include "digital-gamepad/digital-gamepad.hpp"
#include "dualshock/dualshock.hpp"
#include "memory-card/memory-card.hpp"
extern Peripheral peripheral;
