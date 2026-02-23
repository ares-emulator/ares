//MCC - Memory Controller Chip
//Custom logic chip inside the BS-X Satellaview base cartridge

struct MCC {
  ReadableMemory rom;
  WritableMemory psram;

  //mcc.cpp
  auto unload() -> void;
  auto power() -> void;
  auto commit() -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  auto mcuRead(n24 address, n8 data) -> n8;
  auto mcuWrite(n24 address, n8 data) -> void;

  auto mcuAccess(bool mode, n24 address, n8 data) -> n8;
  auto romAccess(bool mode, n24 address, n8 data) -> n8;
  auto psramAccess(bool mode, n24 address, n8 data) -> n8;
  auto exAccess(bool mode, n24 address, n8 data) -> n8;
  auto bsAccess(bool mode, n24 address, n8 data) -> n8;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IRQ {
    n1 flag;    //bit 0
    n1 enable;  //bit 1
  } irq;

  struct Registers {
    n1 mapping;             //bit  2 (0 = ignore A15; 1 = use A15)
    n1 psramEnableLo;       //bit  3
    n1 psramEnableHi;       //bit  4
    n2 psramMapping;        //bits 5-6
    n1 romEnableLo;         //bit  7
    n1 romEnableHi;         //bit  8
    n1 exEnableLo;          //bit  9
    n1 exEnableHi;          //bit 10
    n1 exMapping;           //bit 11
    n1 internallyWritable;  //bit 12 (1 = MCC allows writes to BS Memory Cassette)
    n1 externallyWritable;  //bit 13 (1 = BS Memory Cassette allows writes to flash memory)
  } r, w;

  //bit 14 = commit
  //bit 15 = unknown (test register interface?)
};

extern MCC mcc;
