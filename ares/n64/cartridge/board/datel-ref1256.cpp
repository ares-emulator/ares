struct DATEL_REF1256 : Interface {
  using Interface::Interface;

  //TODO: the GAL on the Game Booster is the exact same as the GAL
  //on the GameShark Pro, so it'd be a good idea to factor out the
  //code and put it in a separate file for both.

#include "gs-eeprom.hpp"

  struct Firmware {
    Cartridge& self;
    Firmware(Cartridge& self) : self(self) {}

    GS_EEPROM eeprom_lo{self}, eeprom_hi{self};
  
    auto size() -> u32 {
      return eeprom_lo.size + eeprom_hi.size;
    }

    auto allocate(u32 capacity, u32 fillWith = ~0) -> void {
      eeprom_lo.allocate(capacity / 2, fillWith);
      eeprom_hi.allocate(capacity / 2, fillWith);
    }

    auto load(VFS::File fp) -> void {
      if(!size()) {
        eeprom_lo.allocate(fp->size() / 2);
        eeprom_hi.allocate(fp->size() / 2);
      }
      for(u32 address = 0; address < min(size(), fp->size()); address += 2) {
        n16 hword = fp->readm(2L);
        eeprom_lo.set(address, hword.bit(0,7 ));
        eeprom_hi.set(address, hword.bit(8,15));
      }
    }

    auto save(VFS::File fp) -> void {
      for(u32 address = 0; address < min(size(), fp->size()); address += 2) {
        n16 hword = 0;
        hword.bit(0,7 ) = eeprom_lo.get(address);
        hword.bit(8,15) = eeprom_hi.get(address);
        fp->writem(hword, 2L);
      }
    }

    auto reset() -> void {
      eeprom_lo.reset();
      eeprom_hi.reset();
    }

    auto clock() -> void {
      return eeprom_lo.clock(), eeprom_hi.clock();
    }

    auto readByte(n20 address) -> u8 {
      if(address.bit(0) == 0) {
        return eeprom_lo.get(address);
      }
      if(address.bit(0) == 1) {
        return eeprom_hi.get(address);
      }
      unreachable;
    }

    auto writeByte(n20 address, u8 data) -> void {
      if(address.bit(0) == 0) {
        return eeprom_lo.set(address, data);
      }
      if(address.bit(0) == 1) {
        return eeprom_hi.set(address, data);
      }
    }

    auto readHalf(n19 address) -> u16 {
      n16 hword = 0;
      hword.bit(0,7 ) = eeprom_lo.read<Byte>(address);
      hword.bit(8,15) = eeprom_hi.read<Byte>(address);
      return hword;
    }

    auto writeHalf(n19 address, n16 data) -> void {
      eeprom_lo.write<Byte>(address, data.bit(0,7 ));
      eeprom_hi.write<Byte>(address, data.bit(8,15));
    }

    auto read(n20 address) -> u16 {
      n16 hword = 0;
      hword.bit(0,7 ) = eeprom_lo.read<Byte>(address >> 1);
      hword.bit(8,15) = eeprom_hi.read<Byte>(address >> 1); 
      return hword;
    }

    auto write(n20 address, n16 data) -> void {
      eeprom_lo.write<Byte>(address >> 1, data.bit(0,7 ));
      eeprom_hi.write<Byte>(address >> 1, data.bit(8,15));
    }

    auto serialize(serializer& s) -> void {
      s(eeprom_lo);
      s(eeprom_hi);
    }
  } firmware{this->cartridge};

  CartridgeSlot slot{"Game Booster Back Cartridge Slot"};
  ares::GameBoy::CartridgeSlot gbslot{"Game Booster Top Cartridge Slot"};

  u8 base;

  //FIXME: this is definitely not how open-bus should work
  u16 last_read = 0;

  struct Debugger {
    DATEL_REF1256& self;
    Debugger(DATEL_REF1256& self) : self(self) {}

    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory firmware;
    } memory;
  } debugger{*this};

  auto load(Node::Object parent) -> void override {
    Interface::load(firmware, "program.eeprom");

    slot.load(parent);
    gbslot.load(parent);

    debugger.load(parent);
  }

  auto unload(Node::Object parent) -> void override {
    debugger.unload(parent);

    gbslot.unload();
    slot.unload();

    firmware.reset();

    cartridge.setClock(0, false);
  }

  auto save() -> void override {
    Interface::save(firmware, "program.eeprom");

    slot.cartridge.save();
    gbslot.cartridge.save();
  }

  auto readBus(u32 address) -> u16 override {
    const u8 bank = address >> 24;

    if(bank == base) {
      return readIO(address & 0x00FFFFFF);
    }

    return slot.cartridge.readHalf(address);
  }

  auto writeBus(u32 address, u16 data) -> void override {
    const u8 bank = address >> 24;

    if(bank == base) {
      return writeIO(address & 0x00FFFFFF, data);
    }

    return slot.cartridge.writeHalf(address, data);
  }

  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override {
    return slot.cartridge.joybusComm(send, recv, input, output);
  }

  auto tickRTC() -> void override {
    return slot.cartridge.tickRTC();
  }

  auto clock() -> void override {
    return firmware.clock();
  }

  auto title() const -> string override {
    return {Interface::title(), " - ", gbslot.cartridge.title()};
  }

  auto cic() const -> string override {
    return slot.cartridge.cic();
  }

  auto power(bool reset) -> void override {
    //should only reset to 0x10 on a power cycle, not a press
    //of the reset button
    if constexpr(Accuracy::Cartridge::GameSharkReset) {
      if(!reset) {
        base = 0x10;
      }
    } else {
      base = 0x10;
    }

    return slot.cartridge.power(reset), gbslot.cartridge.power();
  }
  
  auto serialize(serializer& s) -> void override {
    if(slot.connected()) s(slot.cartridge);
    if(gbslot.connected()) s(gbslot.cartridge);
    s(firmware);
    s(base);
  }
  
  auto readIO(n24 address) -> u16 {
    const u16 unmapped = address & 0xFFFF;

    const u4 section = address >> 20;

    switch(section) {
      case 0: case 1: {
        //boot firmware
        if(address <= 0x00'003f) {
          return firmware.read(address);
        }
        if(address >= 0x00'0040 && address <= 0x00'0fff) {
          //FIXME: haven't actually confirmed this is what happens
          //with base != 0x10
          return slot.cartridge.readHalf(0x1000'0000 | address);
        }
        if(address >= 0x00'1000 && address <= 0x01'ffff) {
          return firmware.read(address);
        }
        return 0x0000;
      } break;

      case 2: {
        //FIXME: check this
        return unmapped;
      } break;

      case 3: {
        //scrambled firmware access
        const n22 offset = address & 0x3FFFFE;

        n22 scrambled = 0;
        scrambled.bit(0 )    = offset.bit(0 );
        scrambled.bit(1 )    = offset.bit(6 );
        scrambled.bit(2 )    = offset.bit(9 );
        scrambled.bit(3 )    = offset.bit(10);
        scrambled.bit(4 )    = offset.bit(4 );
        scrambled.bit(5 )    = offset.bit(5 );
        scrambled.bit(6 )    = offset.bit(1 );
        scrambled.bit(7 )    = offset.bit(7 );
        scrambled.bit(8 )    = offset.bit(8 );
        scrambled.bit(9 )    = offset.bit(3 );
        scrambled.bit(10)    = offset.bit(2 );
        scrambled.bit(11,21) = offset.bit(11,21);

        return firmware.read(scrambled);
      } break;

      case 4: {
        //registers
        return readReg(address);
      } break;

      case 5: {
        //gb slot
        const u22 offset = address & 0x3FFFFE;

        return gbslot.cartridge.read(2, offset >> 1, 0);
      } break;

      case 6: {
        //FIXME: something to do with the scousers updater cart
        return unmapped;
      } break;

      case 7: case 8: case 9: case 0xA: case 0xB: {
        //open bus
        return unmapped;
      } break;

      case 0xC: case 0xD: {
        //direct firmware access
        const n22 offset = address & 0x3FFFFE;

        return firmware.read(offset);
      } break;

      case 0xE: case 0xF: {
        if(address.bit(1) == 0) {
          const n21 vaddr = address & 0x1FFFFC;

          n19 eeprom_addr = 0;
          eeprom_addr.bit(0) = vaddr.bit(20);
          eeprom_addr.bit(1,18) = vaddr.bit(2,19);

          last_read = firmware.readHalf(eeprom_addr);
        }
        return last_read;
      } break;
    }
    unreachable;
  }

  auto writeIO(n24 address, u16 data) -> void {
    const u4 section = address >> 20;

    switch(section) {
      case 0: case 1: {
        //boot firmware
        //(not writable)
        return;
      } break;

      case 2: {
        //FIXME: check this
        //(not writable)
        return;
      } break;

      case 3: {
        //scrambled firmware access
        //(not writable)
        return;
      } break;

      case 4: {
        //registers
        return writeReg(address, data);
      } break;

      case 5: {
        //gb slot
        const u22 offset = address & 0x3FFFFE;

        return gbslot.cartridge.write(2, offset >> 1, data);
      } break;

      case 6: {
        //FIXME: something to do with the scousers updater cart
        return;
      } break;

      case 7: case 8: case 9: case 0xA: case 0xB: {
        //open bus
        return;
      } break;

      case 0xC: case 0xD: {
        //direct firmware access
        //(not writable)
        return;
      } break;

      case 0xE: case 0xF: {
        if(address.bit(1) == 0) {
          const n21 vaddr = address & 0x1FFFFC;

          n19 eeprom_addr = 0;
          eeprom_addr.bit(0) = vaddr.bit(20);
          eeprom_addr.bit(1,18) = vaddr.bit(2,19);

          firmware.writeHalf(eeprom_addr, data);
        }
        return;
      } break;
    }
    unreachable;
  }

  auto readReg(u24 address) -> u16 {
    const u16 unmapped = address & 0xFFFF;

    //FIXME: check to make sure the address decoding is actually correct
    if(address >= 0x40'0000 && address <= 0x40'03ff) {
      //parallel port
      //FIXME: implement something for the parallel port
      return 0x0000;
    }

    if(address >= 0x40'0400 && address <= 0x40'05ff) {
      //base address
      //returns open bus on read
    }

    if(address >= 0x40'0600 && address <= 0x40'07ff) {
      //shift register unlock
      //returns open bus on read
    }

    if(address >= 0x40'0800 && address <= 0x40'09ff) {
      //shift register clock/data
      //returns open bus on read
    }

    return unmapped;
  }

  auto writeReg(u24 address, u16 data) -> void {
    //FIXME: check to make sure the address decoding is actually correct
    if(address >= 0x40'0000 && address <= 0x40'03ff) {
      //parallel port
      //(writes do nothing)
      return;
    }

    if(address >= 0x40'0400 && address <= 0x40'05ff) {
      //base address
      //updates immediately
      base = data & 0xFF;
      return;
    }

    if(address >= 0x40'0600 && address <= 0x40'07ff) {
      //shift register unlock
      //FIXME: implement
      return;
    }
    
    if(address >= 0x40'0800 && address <= 0x40'09ff) {
      //shift register clock/data
      //FIXME: implement
      return;
    }

    return;
  }
};

auto DATEL_REF1256::Debugger::load(Node::Object parent) -> void {
  memory.firmware = parent->append<Node::Debugger::Memory>("GameShark Firmware");
  memory.firmware->setSize(self.firmware.size());
  memory.firmware->setRead([&](u32 address) -> u8 {
    return self.firmware.readByte(address);
  });
  memory.firmware->setWrite([&](u32 address, u8 data) -> void {
    return self.firmware.writeByte(address, data);
  });
}

auto DATEL_REF1256::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.firmware);
  memory.firmware.reset();
}

#define BOARD DATEL_REF1256

#include "gs-eeprom.cpp"

#undef BOARD
