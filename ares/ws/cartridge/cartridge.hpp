struct Cartridge : Thread, IO {
  Node::Peripheral node;

  auto manifest() const { return information.manifest; }
  auto name() const { return information.name; }
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
    string manifest;
    string name;
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

  struct Memory {
    n8* data = nullptr;
    u32 size = 0;
    u32 mask = 0;
  };

  struct RTC : Memory {
    n8 command;
    n4 index;

    n8 alarm;
    n8 alarmHour;
    n8 alarmMinute;

    auto year()    -> n8& { return data[0]; }
    auto month()   -> n8& { return data[1]; }
    auto day()     -> n8& { return data[2]; }
    auto weekday() -> n8& { return data[3]; }
    auto hour()    -> n8& { return data[4]; }
    auto minute()  -> n8& { return data[5]; }
    auto second()  -> n8& { return data[6]; }
  };

  Memory rom;
  Memory ram;
  EEPROM eeprom;
  RTC rtc;
};

#include "slot.hpp"
extern Cartridge& cartridge;
