struct MROM {
  //mrom.cpp
  auto read(u32 mode, n32 address) -> n32;
  auto write(u32 mode, n32 address, n32 word) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n8* data = nullptr;
  u32 size;
  u32 mask;
} mrom;

struct SRAM {
  //sram.cpp
  auto read(u32 mode, n32 address) -> n32;
  auto write(u32 mode, n32 address, n32 word) -> void;

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
