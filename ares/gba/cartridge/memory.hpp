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
  auto write(n32 address, n8 byte) -> void;

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

struct RTC : S3511A, Thread {
  //rtc.cpp
  auto irqLevel(bool value) -> void override;
  auto power() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  bool irq;
} rtc;
