struct Cartridge : Thread, IO {
  Node::Peripheral node;
  VFS::Pak pak;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  EEPROM eeprom;
  struct RTC : Memory::Writable<n8> {
    n8 command;
    n4 index;

    n8 alarm;
    n8 alarmHour;
    n8 alarmMinute;

    auto year()    -> n8& { return operator[](0); }
    auto month()   -> n8& { return operator[](1); }
    auto day()     -> n8& { return operator[](2); }
    auto weekday() -> n8& { return operator[](3); }
    auto hour()    -> n8& { return operator[](4); }
    auto minute()  -> n8& { return operator[](5); }
    auto second()  -> n8& { return operator[](6); }
  } rtc;

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
  auto romRead(n20 address) -> n8;
  auto romWrite(n20 address, n8 data) -> void;

  auto ramRead(n20 address) -> n8;
  auto ramWrite(n20 address, n8 data) -> void;

  //rtc.cpp
  auto rtcLoad() -> void;
  auto rtcSave() -> void;
  auto rtcTickSecond() -> void;
  auto rtcCheckAlarm() -> void;
  auto rtcStatus() -> n8;
  auto rtcCommand(n8 data) -> void;
  auto rtcRead() -> n8;
  auto rtcWrite(n8 data) -> void;

  //io.cpp
  auto portRead(n16 address) -> n8 override;
  auto portWrite(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string title;
    string orientation = "Horizontal";
  } information;

  struct Registers {
    //$00c0  BANK_ROM2
    n8 romBank2 = 0xff;

    //$00c1  BANK_SRAM
    n8 sramBank = 0xff;

    //$00c2  BANK_ROM0
    n8 romBank0 = 0xff;

    //$00c3  BANK_ROM1
    n8 romBank1 = 0xff;

    //$00cc  GPO_EN
    n8 gpoEnable;

    //$00cd  GPO_DATA
    n8 gpoData;
  } r;
};

#include "slot.hpp"
extern Cartridge& cartridge;
