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
  auto readRom(u32 mode, n32 address) -> n16 {
    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.read();
    if(has.rtc) {
      if((address & 0x1fffffe) == 0xc4) return gpio.readData();
      if((address & 0x1fffffe) == 0xc6) return gpio.readDirection();
      if((address & 0x1fffffe) == 0xc8) return gpio.readControl();
    }

    n32 romAddr;
    if constexpr(!UseDebugger) romAddr = mrom.burstAddr(mode, address);
    if constexpr( UseDebugger) romAddr = address & 0x1ff'fffe;
    n16 half = mrom.read(mode, romAddr);
    if constexpr(!UseDebugger) mrom.pageAddr++;
    return half;
  }

  auto readBackup(n32 address) -> n8 {
    n8 byte = 0xff;
    if(has.sram) byte = sram.read(address);
    if(has.flash) byte = flash.read(address);
    return byte;
  }

  auto writeRom(u32 mode, n32 address, n16 half) -> void {
    if(has.eeprom && (address & eeprom.mask) == eeprom.test) return eeprom.write(half & 1);

    if(has.rtc) {
      if((address & 0x1fffffe) == 0xc4) return gpio.writeData(half);
      if((address & 0x1fffffe) == 0xc6) return gpio.writeDirection(half);
      if((address & 0x1fffffe) == 0xc8) return gpio.writeControl(half);
    }

    n32 romAddr = mrom.burstAddr(mode, address);
    mrom.write(mode, romAddr, half);
    mrom.pageAddr++;
    return;
  }

  auto writeBackup(n32 address, n8 byte) -> void {
    if(has.sram) return sram.write(address, byte);
    if(has.flash) return flash.write(address, byte);
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
    n1 rtc;
  } has;
};

#include "slot.hpp"
extern Cartridge& cartridge;
