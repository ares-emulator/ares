struct MROM {
  //mrom.cpp
  auto read(u32 mode, n32 address) -> n16;
  auto write(u32 mode, n32 address, n16 half) -> void;
  auto burstAddr(u32 mode, n32 address) -> n32;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  Memory::Readable<n16> data;
  u32 size;
  u32 mask;
  n16 pageAddr;
  n1  burst;
  bool mirror;
} mrom;

struct SRAM {
  //sram.cpp
  auto read(n32 address) -> n8;
  auto write(n32 address, n32 word) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8* data = nullptr;
  u32 size;
  u32 mask;
} sram;

struct EEPROM {
  //eeprom.cpp
  auto read(u32 address) -> bool;
  auto write(u32 address, bool bit) -> void;

  auto read() -> bool;
  auto write(bool bit) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8* data = nullptr;
  u32 size;
  u32 mask;
  u32 test;
  u32 bits;

  enum class Mode : u32 {
    Wait, Command, ReadAddress, ReadValidate, ReadData, WriteAddress, WriteData, WriteValidate
  } mode;
  u32 offset;
  u32 address;
  u32 addressbits;
} eeprom;

struct FLASH {
  //flash.cpp
  auto read(n16 address) -> n8;
  auto write(n16 address, n8 byte) -> void;

  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8* data = nullptr;
  u32 size;
  string manufacturer;

  n16 id;

  bool unlockhi;
  bool unlocklo;
  bool idmode;
  bool erasemode;
  bool bankselect;
  bool writeselect;
  bool bank;
} flash;

struct GPIO {
  //gpio.cpp
  auto readData() -> n4;
  auto readDirection() -> n4;
  auto readControl() -> n1;
  auto writeData(n4 data) -> void;
  auto writeDirection(n4 data) -> void;
  auto writeControl(n1 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n4 latch;
  n4 direction;
  n1 readEnable;
} gpio;

struct RTC : Thread {
  // bytes  0- 6: date/time data, BCD
  // bytes  7- 7: status register
  // bytes  8-15: last timestamp
  // bytes 16-17: alarm register
  n8* data = nullptr;
  const u32 size = 18;

  //rtc.cpp
  auto load() -> void;
  auto save() -> void;
  auto tickSecond() -> void;
  auto checkAlarm() -> void;
  auto readSIO() -> n1;
  auto writeCS(n1 data) -> void;
  auto writeSIO(n1 data) -> void;
  auto writeSCK(n1 data) -> void;
  auto writeCommand() -> void;
  auto readRegister() -> void;
  auto writeRegister() -> void;
  auto initRegs(bool reset) -> void;
  auto power() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  //pin states
  n1 cs;
  n1 sioIn;
  n1 sioOut;
  n1 sck;

  //buffered command data
  n8 inBuffer;
  n8 outBuffer;
  n3 shift;
  n3 index;

  //internal state
  n1  rwSelect;
  n3  regSelect;
  n1  cmdLatched;
  n15 counter;

  auto year()        -> n8& { return data[ 0]; }
  auto month()       -> n8& { return data[ 1]; }
  auto day()         -> n8& { return data[ 2]; }
  auto weekday()     -> n8& { return data[ 3]; }
  auto hour()        -> n8& { return data[ 4]; }
  auto minute()      -> n8& { return data[ 5]; }
  auto second()      -> n8& { return data[ 6]; }
  auto status()      -> n8& { return data[ 7]; }
  auto alarmHour()   -> n8& { return data[16]; }
  auto alarmMinute() -> n8& { return data[17]; }
} rtc;
