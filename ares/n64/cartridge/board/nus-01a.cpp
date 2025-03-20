struct NUS_01A : Interface {
  using Interface::Interface;
  Memory::Readable16 rom;
  Memory::Writable16 eeprom;

  struct Debugger {
    NUS_01A& self;
    Debugger(NUS_01A& self) : self(self) {}
  
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory rom;
      Node::Debugger::Memory eeprom;
    } memory;
  } debugger{*this};

  auto load(Node::Object parent) -> void override {
    Interface::load(rom, "program.rom");
    Interface::load(eeprom, "save.eeprom");

    debugger.load(parent);
  }

  auto unload(Node::Object parent) -> void override {
    debugger.unload(parent);

    rom.reset();
  }

  auto save() -> void override {
    Interface::save(eeprom, "save.eeprom");
  }
  
  auto readBus(u32 address) -> u16 override {
    const u16 unmapped = address & 0xFFFF;

    if(address >= 0x1000'0000 && address <= 0x1000'0000 + rom.size - 1) {
      return rom.read<Half>(address);
    }

    return unmapped;
  }

  auto writeBus(u32 address, u16 data) -> void override {
    if(address >= 0x1000'0000 && address <= 0x1000'0000 + rom.size - 1) {
      return rom.write<Half>(address, data);
    }
  }

  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override {
    n1 valid = 0, over = 0;

    valid = Interface::joybusEeprom(eeprom, send, recv, input, output);
  
    n2 status;
    status.bit(0) = valid;
    status.bit(1) = over;
    return status;
  }

  auto serialize(serializer& s) -> void override {
    s(eeprom);
  }
};

auto NUS_01A::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(self.rom.size);
  memory.rom->setRead([&](u32 address) -> u8 {
    return self.rom.read<Byte>(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return self.rom.write<Byte>(address, data);
  });

  memory.eeprom = parent->append<Node::Debugger::Memory>("Cartridge EEPROM");
  memory.eeprom->setSize(self.eeprom.size);
  memory.eeprom->setRead([&](u32 address) -> u8 {
    return self.eeprom.read<Byte>(address);
  });
  memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
    return self.eeprom.write<Byte>(address, data);
  });
}

auto NUS_01A::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.rom);
  parent->remove(memory.eeprom);
  memory.rom.reset();
  memory.eeprom.reset();
}