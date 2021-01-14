auto PCD::Debugger::load(Node::Object parent) -> void {
  memory.wram = parent->append<Node::Debugger::Memory>("CD WRAM");
  memory.wram->setSize(64_KiB);
  memory.wram->setRead([&](u32 address) -> u8 {
    return pcd.wram.read(address);
  });
  memory.wram->setWrite([&](u32 address, u8 data) -> void {
    return pcd.wram.write(address, data);
  });

  memory.bram = parent->append<Node::Debugger::Memory>("CD BRAM");
  memory.bram->setSize(2_KiB);
  memory.bram->setRead([&](u32 address) -> u8 {
    return pcd.bram.read(address);
  });
  memory.bram->setWrite([&](u32 address, u8 data) -> void {
    return pcd.bram.write(address, data);
  });

  if(Model::PCEngineDuo()) {
    memory.sram = parent->append<Node::Debugger::Memory>("CD SRAM");
    memory.sram->setSize(192_KiB);
    memory.sram->setRead([&](u32 address) -> u8 {
      return pcd.sram.read(address);
    });
    memory.sram->setWrite([&](u32 address, u8 data) -> void {
      return pcd.sram.write(address, data);
    });
  }

  memory.adpcm = parent->append<Node::Debugger::Memory>("CD ADPCM");
  memory.adpcm->setSize(64_KiB);
  memory.adpcm->setRead([&](u32 address) -> u8 {
    return pcd.adpcm.memory.read(address);
  });
  memory.adpcm->setWrite([&](u32 address, u8 data) -> void {
    return pcd.adpcm.memory.write(address, data);
  });
}
