struct Cartridge : Thread, IO {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  EEPROM eeprom;
  struct RTC;

  struct Debugger {
    Cartridge& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory ram;
      Node::Debugger::Memory eeprom;
    } memory;
  } debugger{*this};

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
  auto writeROM(n20 address, n8 data) -> void;

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

  struct RTC {
    Cartridge& self;
    Memory::Writable<n8> ram;

    //rtc.cpp
    auto load() -> void;
    auto save() -> void;
    auto tickSecond() -> void;
    auto checkAlarm() -> void;
    auto status() -> n8;
    auto execute(n8 data) -> void;
    auto read() -> n8;
    auto write(n8 data) -> void;
    auto power() -> void;

    n8 command;
    n4 index;

    n8 alarm;
    n8 alarmHour;
    n8 alarmMinute;

    auto year()    -> n8& { return ram[0]; }
    auto month()   -> n8& { return ram[1]; }
    auto day()     -> n8& { return ram[2]; }
    auto weekday() -> n8& { return ram[3]; }
    auto hour()    -> n8& { return ram[4]; }
    auto minute()  -> n8& { return ram[5]; }
    auto second()  -> n8& { return ram[6]; }
  } rtc{*this};

  struct IO {
    n8 romBank2 = 0xff;
    n8 sramBank = 0xff;
    n8 romBank0 = 0xff;
    n8 romBank1 = 0xff;
    n8 gpoEnable;
    n8 gpoData;
  } io;
};

#include "slot.hpp"
extern Cartridge& cartridge;
