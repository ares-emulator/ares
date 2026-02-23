auto System::Debugger::load(Node::Object parent) -> void {
  memory.bios = parent->append<Node::Debugger::Memory>("System BIOS");
  memory.bios->setSize(128_KiB);
  memory.bios->setRead([&](u32 address) -> u8 {
    return system.bios[address >> 1].byte(!(address & 1));
  });
  memory.bios->setWrite([&](u32 address, u8 data) -> void {
    system.bios[address >> 1].byte(!(address & 1)) = data;
  });

//memory.srom = parent->append<Node::Debugger::Memory>("System Static ROM");

  memory.wram = parent->append<Node::Debugger::Memory>("System Work RAM");
  memory.wram->setSize(64_KiB);
  memory.wram->setRead([&](u32 address) -> u8 {
    return system.wram[address >> 1].byte(!(address & 1));
  });
  memory.wram->setWrite([&](u32 address, u8 data) -> void {
    system.wram[address >> 1].byte(!(address & 1)) = data;
  });

  if(NeoGeo::Model::NeoGeoMVS()) {
    memory.sram = parent->append<Node::Debugger::Memory>("System Backup RAM");
    memory.sram->setSize(64_KiB);
    memory.sram->setRead([&](u32 address) -> u8 {
      return system.sram[address >> 1].byte(!(address & 1));
    });
    memory.sram->setWrite([&](u32 address, u8 data) -> void {
      system.sram[address >> 1].byte(!(address & 1)) = data;
    });
  }
}

auto System::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.bios);
//parent->remove(memory.srom);
  parent->remove(memory.wram);
  if(NeoGeo::Model::NeoGeoMVS()) parent->remove(memory.sram);
  memory.bios.reset();
//memory.srom.reset();
  memory.wram.reset();
  if(NeoGeo::Model::NeoGeoMVS()) memory.sram.reset();
}
