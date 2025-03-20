struct DATEL_REF1329 : Interface {
  using Interface::Interface;
  //FIXME: replace this with two EEPROMs
  Memory::Writable16 firmware;
  CartridgeSlot slot{"GameShark Cartridge Slot"};

  u8 base;

  struct Debugger {
    DATEL_REF1329& self;
    Debugger(DATEL_REF1329& self) : self(self) {}

    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory firmware;
    } memory;
  } debugger{*this};

  auto load(Node::Object parent) -> void override {
    Interface::load(firmware, "program.eeprom");

    slot.load(parent);

    debugger.load(parent);
  }

  auto unload(Node::Object parent) -> void override {
    debugger.unload(parent);

    slot.unload();

    firmware.reset();
  }

  auto save() -> void override {
    Interface::save(firmware, "program.eeprom");

    slot.cartridge.save();
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

  auto title() const -> string override {
    return {"GameShark Pro - ", slot.cartridge.title()};
  }

  auto cic() const -> string override {
    return slot.cartridge.cic();
  }

  auto power(bool reset) -> void override {
    if(!reset) {
      //FIXME: should only reset to 0x10 on a power cycle, not a press
      //of the reset button
      base = 0x10;
    }

    return slot.cartridge.power(reset);
  }
  
  auto serialize(serializer& s) -> void override {
    if(slot.connected()) s(slot.cartridge);
    s(firmware);
    s(base);
  }
  
  auto readIO(u24 address) -> u16 {
    const u16 unmapped = address & 0xFFFF;

    const u4 section = address >> 20;

    switch(section) {
      case 0: case 1: {
        //boot firmware
        if(address <= 0x00'003f) {
          return firmware.read<Half>(address);
        }
        if(address >= 0x00'0040 && address <= 0x00'0fff) {
          //FIXME: haven't actually confirmed this is what happens
          //with base != 0x10
          return slot.cartridge.readHalf(0x1000'0000 | address);
        }
        if(address >= 0x00'1000 && address <= 0x01'ffff) {
          return firmware.read<Half>(address);
        }
        return 0x0000;
      } break;

      case 2: {
        //FIXME: check this
        return unmapped;
      } break;

      case 3: {
        //FIXME: this is actually a strange mapping from the EEPROMs
        return unmapped;
      } break;

      case 4: {
        //registers
        return readReg(address);
      } break;

      case 5: {
        //parallel port
        return unmapped;
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
        const u22 offset = address & 0x3FFFFF;

        if(offset <= firmware.size - 1) {
          return firmware.read<Half>(offset);
        }

        return unmapped;
      } break;

      case 0xE: case 0xF: {
        //FIXME: replace this with proper EEPROM code

        const n21 vaddr = address & 0x1FFFFF;

        n17 eeprom_addr = 0;
        eeprom_addr.bit(0) = vaddr.bit(20);
        eeprom_addr.bit(1,16) = vaddr.bit(2,17);

        const n18 firmware_addr = eeprom_addr * 2;

        if(firmware_addr <= firmware.size - 1) {
          return firmware.read<Half>(firmware_addr);
        }
        return unmapped;
      } break;
    }
    unreachable;
  }

  auto writeIO(u24 address, u16 data) -> void {
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
        //FIXME: this is actually a strange mapping from the EEPROMs
        //(not writable)
        return;
      } break;

      case 4: {
        //registers
        return writeReg(address, data);
      } break;

      case 5: {
        //parallel port
        //FIXME: implement something for the parallel port
        return;
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
        //FIXME: replace this with proper EEPROM code

        const n21 vaddr = address & 0x1FFFFF;

        n17 eeprom_addr = 0;
        eeprom_addr.bit(0) = vaddr.bit(20);
        eeprom_addr.bit(1,16) = vaddr.bit(2,17);

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

auto DATEL_REF1329::Debugger::load(Node::Object parent) -> void {
  memory.firmware = parent->append<Node::Debugger::Memory>("GameShark Firmware");
  memory.firmware->setSize(self.firmware.size);
  memory.firmware->setRead([&](u32 address) -> u8 {
    return self.firmware.read<Byte>(address);
  });
  memory.firmware->setWrite([&](u32 address, u8 data) -> void {
    return self.firmware.write<Byte>(address, data);
  });
}

auto DATEL_REF1329::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.firmware);
  memory.firmware.reset();
}
