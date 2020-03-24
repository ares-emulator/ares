struct MROM {
  //mrom.cpp
  auto read(uint mode, uint32 address) -> uint32;
  auto write(uint mode, uint32 address, uint32 word) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint8* data = nullptr;
  uint size;
  uint mask;
} mrom;

struct SRAM {
  //sram.cpp
  auto read(uint mode, uint32 address) -> uint32;
  auto write(uint mode, uint32 address, uint32 word) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint8* data = nullptr;
  uint size;
  uint mask;
} sram;

struct EEPROM {
  //eeprom.cpp
  auto read(uint address) -> bool;
  auto write(uint address, bool bit) -> void;

  auto read() -> bool;
  auto write(bool bit) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint8* data = nullptr;
  uint size;
  uint mask;
  uint test;
  uint bits;

  enum class Mode : uint {
    Wait, Command, ReadAddress, ReadValidate, ReadData, WriteAddress, WriteData, WriteValidate
  } mode;
  uint offset;
  uint address;
  uint addressbits;
} eeprom;

struct FLASH {
  //flash.cpp
  auto read(uint16 address) -> uint8;
  auto write(uint16 address, uint8 byte) -> void;

  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  uint8* data = nullptr;
  uint size;
  string manufacturer;

  uint16 id;

  bool unlockhi;
  bool unlocklo;
  bool idmode;
  bool erasemode;
  bool bankselect;
  bool writeselect;
  bool bank;
} flash;
