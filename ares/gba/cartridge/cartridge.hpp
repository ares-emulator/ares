struct Cartridge {
  Node::Peripheral node;
  VFS::Pak pak;

  #include "memory.hpp"

  auto title() const -> string { return information.title; }

  //cartridge.cpp
  Cartridge();
  ~Cartridge();

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  template<bool UseDebugger>
  auto readRom(u32 mode, n32 address) -> n32 {
    if(mode & Word) {
      n32 word = 0;
      word |= readRom<UseDebugger>(mode & ~Word | Half, (address & ~3) + 0) <<  0;
      word |= readRom<UseDebugger>(Sequential | Half, (address & ~3) + 2) << 16;
      return word;
    }

    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.read();

    n32 romAddr;
    if constexpr(!UseDebugger) romAddr = mrom.burstAddr(mode, address);
    if constexpr( UseDebugger) romAddr = address & 0x1ff'fffe;
    n16 half = mrom.read(mode, romAddr);
    if constexpr(!UseDebugger) mrom.pageAddr++;

    if(mode & Byte) return half.byte(address & 1);
    return half;
  }

  auto readBackup(u32 mode, n32 address) -> n32 {
    n32 word = 0xff;
    if(has.sram) word = sram.read(address);
    if(has.flash) word = flash.read(address);
    word *= 0x01010101;
    return word;
  }

  auto writeRom(u32 mode, n32 address, n32 word) -> void {
    if(mode & Word) {
      writeRom(mode & ~Word | Half, (address & ~3) + 0, word >> 0);
      writeRom(Sequential | Half, (address & ~3) + 2, word >> 16);
    }

    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.write(word & 1);

    n32 romAddr = mrom.burstAddr(mode, address);
    mrom.write(mode, romAddr, word);
    mrom.pageAddr++;
    return;
  }

  auto writeBackup(u32 mode, n32 address, n32 word) -> void {
    if(mode & Word) word = word >> (8 * (address & 3));
    if(mode & Half) word = word >> (8 * (address & 1));
    if(has.sram) return sram.write(address, word);
    if(has.flash) return flash.write(address, word);
  }

  auto serialize(serializer&) -> void;

private:
  struct Information {
    string title;
  } information;

  struct Has {
    n1 sram;
    n1 eeprom;
    n1 flash;
  } has;
};

#include "slot.hpp"
extern Cartridge& cartridge;
