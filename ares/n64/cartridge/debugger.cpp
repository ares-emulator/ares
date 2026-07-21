auto Cartridge::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge ROM");
  memory.rom->setSize(cartridge.rom.size);
  memory.rom->setRead([&](u32 address) -> u8 {
    return cartridge.rom.read<Byte>(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    return cartridge.rom.write<Byte>(address, data);
  });

  if(cartridge.ram) {
    memory.ram = parent->append<Node::Debugger::Memory>("Cartridge SRAM");
    memory.ram->setSize(cartridge.ram.size);
    memory.ram->setRead([&](u32 address) -> u8 {
      return cartridge.ram.read<Byte>(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return cartridge.ram.write<Byte>(address, data);
    });
  }

  if(cartridge.eeprom) {
    memory.eeprom = parent->append<Node::Debugger::Memory>("Cartridge EEPROM");
    memory.eeprom->setSize(cartridge.eeprom.size);
    memory.eeprom->setRead([&](u32 address) -> u8 {
      return cartridge.eeprom.read<Byte>(address);
    });
    memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
      return cartridge.eeprom.write<Byte>(address, data);
    });
  }

  if(cartridge.flash) {
    memory.flash = parent->append<Node::Debugger::Memory>("Cartridge Flash");
    memory.flash->setSize(cartridge.flash.size);
    memory.flash->setRead([&](u32 address) -> u8 {
      return static_cast<::ares::Nintendo64::Memory::Writable&>(cartridge.flash).read<Byte>(address);
    });
    memory.flash->setWrite([&](u32 address, u8 data) -> void {
      return static_cast<::ares::Nintendo64::Memory::Writable&>(cartridge.flash).write<Byte>(address, data);
    });
    tracer.flash = parent->append<Node::Debugger::Tracer::Notification>("Flash", "Cartridge");
  }
}

auto Cartridge::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.rom);
  parent->remove(memory.ram);
  parent->remove(memory.eeprom);
  parent->remove(memory.flash);
  parent->remove(tracer.flash);
  memory.rom.reset();
  memory.ram.reset();
  memory.eeprom.reset();
  memory.flash.reset();
  tracer.flash.reset();
}

auto Cartridge::Debugger::flash(u32 command) -> void {
  if(unlikely(!tracer.flash) || unlikely(!tracer.flash->enabled())) return;

  auto& flash = cartridge.flash;
  u8 cmd = command >> 24;
  string name;
  switch(cmd) {
  case 0x3c: name = "ChipEraseSetup"; break;
  case 0x4b: name = {"SectorEraseSetup page=", hex(command & 0x3ff, 3L)}; break;
  case 0x78: name = "Erase"; break;
  case 0xa5: name = {"ProgramPage page=", hex(command & 0x3ff, 3L)}; break;
  case 0xb4: name = "LoadBytePage"; break;
  case 0xd2: name = "Status"; break;
  case 0xe1: name = "SiliconID"; break;
  case 0xf0: name = "ReadArray"; break;
  default:   name = {"Unknown cmd=", hex(cmd, 2L)}; break;
  }

  static const char* modes[] = {"ReadArray", "Status", "SiliconID", "LoadBytePage"};
  static const char* busys[] = {"None", "Erase", "Program"};
  tracer.flash->notify({
    name, " cmd=", hex(command, 8L),
    " mode=", modes[(u32)flash.mode],
    " busy=", busys[(u32)flash.busy],
    " status=", hex(flash.status.data, 2L),
  });
}
