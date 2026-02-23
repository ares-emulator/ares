//MaskROMs supported:
//  Sharp LH5S4TNI (MaskROM 512K x 8-bit) [BSMC-CR-01: BSMC-ZS5J-JPN, BSMC-YS5J-JPN]
//  Sharp LH534VNF (MaskROM 512K x 8-bit) [BSMC-BR-01: BSMC-ZX3J-JPN]

//Flash chips supported: (16-bit modes unsupported)
//  Sharp LH28F800SUT-ZI (Flash 16 x 65536 x 8-bit) [BSMC-AF-01: BSMC-HM-JPN]
//  Sharp LH28F016SU ??? (Flash 32 x 65536 x 8-bit) [unreleased: experimental]
//  Sharp LH28F032SU ??? (Flash 64 x 65536 x 8-bit) [unreleased: experimental]

//unsupported:
//  Sharp LH28F400SU ??? (Flash 32 x 16384 x 8-bit) [unreleased] {vendor ID: 0x00'b0; device ID: 0x66'21}

//notes:
//timing emulation is only present for block erase commands
//other commands generally complete so quickly that it's unnecessary (eg 70-120ns for writes)
//suspend, resume, abort, ready/busy modes are not supported

struct BSMemoryCartridge : Thread {
  Node::Peripheral node;
  VFS::Pak pak;
  u32 ROM = 1;

  auto title() const -> string { return information.title; }
  auto size() const -> u32 { return memory.size(); }
  auto writable() const { return pin.writable; }
  auto writable(bool writable) { pin.writable = !ROM && writable; }

  //bsmemory.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  BSMemoryCartridge();
  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto power() -> void;
  auto save() -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  WritableMemory memory;

  struct {
    string title;
  } information;

private:
  struct Pin {
    n1 writable;  // => /WP
  } pin;

  struct Chip {
    n16 vendor;
    n16 device;
    n48 serial;
  } chip;

  struct Page {
    BSMemoryCartridge* self = nullptr;

    auto swap() -> void;
    auto read(n8 address) -> n8;
    auto write(n8 address, n8 data) -> void;

    n8 buffer[2][256];
  } page;

  struct BlockInformation {
    BSMemoryCartridge* self = nullptr;

    auto bits() const -> u32;
    auto bytes() const -> u32;
    auto count() const -> u32;
  };

  struct Block : BlockInformation {
    auto read(n24 address) -> n8;
    auto write(n24 address, n8 data) -> void;
    auto erase() -> void;
    auto lock() -> void;
    auto update() -> void;

    n4  id;
    n32 erased;
    n1  locked;
    n1  erasing;

    struct Status {
      auto operator()() -> n8;

      n1 vppLow;
      n1 queueFull;
      n1 aborted;
      n1 failed;
      n1 locked = 1;
      n1 ready = 1;
    } status;
  } blocks[64];  //8mbit = 16; 16mbit = 32; 32mbit = 64

  struct Blocks : BlockInformation {
    auto operator()(n6 id) -> Block&;
  } block;

  struct Compatible {
    struct Status {
      auto operator()() -> n8;

      n1 vppLow;
      n1 writeFailed;
      n1 eraseFailed;
      n1 eraseSuspended;
      n1 ready = 1;
    } status;
  } compatible;

  struct Global {
    struct Status {
      auto operator()() -> n8;

      n1 page;
      n1 pageReady = 1;
      n1 pageAvailable = 1;
      n1 queueFull;
      n1 sleeping;
      n1 failed;
      n1 suspended;
      n1 ready = 1;
    } status;
  } global;

  struct Mode { enum : u32 {
    Flash,
    Chip,
    Page,
    CompatibleStatus,
    ExtendedStatus,
  };};
  n3 mode;

  struct ReadyBusyMode { enum : u32 {
    EnableToLevelMode,
    PulseOnWrite,
    PulseOnErase,
    Disable,
  };};
  n2 readyBusyMode;

  struct Queue {
    auto flush() -> void;
    auto pop() -> void;
    auto push(n24 address, n8 data) -> void;
    auto size() -> u32;
    auto address(u32 index) -> n24;
    auto data(u32 index) -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct History {
      n1  valid;
      n24 address;
      n8  data;
    } history[4];
  } queue;

  auto failed() -> void;
};

#include "slot.hpp"
extern BSMemoryCartridge& bsmemory;
