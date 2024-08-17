struct Cartridge : IO, Thread {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Writable<n8> rom;
  Memory::Writable<n8> ram;
  EEPROM eeprom;
  struct RTC;
  struct FLASH;

  struct Debugger {
    Cartridge& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto ports() -> string;

    struct Properties {
      Node::Debugger::Properties ports;
    } properties;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory ram;
      Node::Debugger::Memory eeprom;
    } memory;
  } debugger{*this};

  struct Has {
    n1 sram;
    n1 eeprom;
    n1 rtc;
    n1 flash;
    n1 karnak;
  } has;

  auto title() const { return information.title; }
  auto orientation() const { return information.orientation; }

  //cartridge.cpp
  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  //memory.cpp
  auto readROM(n20 address) -> n8;
  auto readRAM(n20 address) -> n8;
  auto writeRAM(n20 address, n8 data) -> void;

  //io.cpp
  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string title;
    string orientation = "Horizontal";
  } information;

  struct RTC : Thread {
    // bytes  0- 6: date/time data, BCD
    // bytes  7- 7: status register
    // bytes  8-15: last timestamp
    // bytes 16-17: alarm register
    Memory::Writable<n8> ram;

    //rtc.cpp
    auto load() -> void;
    auto save() -> void;
    auto tickSecond() -> void;
    auto checkAlarm() -> void;
    auto controlRead() -> n8;
    auto controlWrite(n5 data) -> void;
    auto fetch() -> void;
    auto read() -> n8;
    auto initRegs(bool reset) -> void;
    auto write(n8 data) -> void;
    auto power() -> void;
    auto reset() -> void;
    auto main() -> void;
    auto step(u32 clocks) -> void;

    //serialization.cpp
    auto serialize(serializer& s) -> void;

    n4 command;
    n1 active;
    n4 index;
    n8 fetchedData;
    n15 counter;

    auto year()        -> n8& { return ram[ 0]; }
    auto month()       -> n8& { return ram[ 1]; }
    auto day()         -> n8& { return ram[ 2]; }
    auto weekday()     -> n8& { return ram[ 3]; }
    auto hour()        -> n8& { return ram[ 4]; }
    auto minute()      -> n8& { return ram[ 5]; }
    auto second()      -> n8& { return ram[ 6]; }
    auto status()      -> n8& { return ram[ 7]; }
    auto alarmHour()   -> n8& { return ram[16]; }
    auto alarmMinute() -> n8& { return ram[17]; }
  } rtc;

  struct KARNAK : Thread {
    //karnak.cpp
    auto power() -> void;
    auto reset() -> void;
    auto main() -> void;
    auto step(u32 clocks) -> void;

    //serialization.cpp
    auto serialize(serializer& s) -> void;

    n1 timerEnable;
    n7 timerPeriod;
    n9 timerCounter;
  } karnak;

  struct FLASH {
    Cartridge& self;

    //flash.cpp
    auto read(n19 address, bool words) -> n16;
    auto write(n19 address, n8 byte) -> void;
    auto power() -> void;

    n2 unlock;
    bool idmode;
    bool programmode;
    bool fastmode;
    bool erasemode;
  } flash{*this};

  struct IO {
    n16 romBank2 = 0xffff;
    n16 sramBank = 0xffff;
    n16 romBank0 = 0xffff;
    n16 romBank1 = 0xffff;
    n8 gpoEnable;
    n8 gpoData;
    n1 flashEnable;
  } io;

  n16 openbus;
};

#include "slot.hpp"
extern Cartridge& cartridge;
