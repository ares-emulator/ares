auto PCD::Debugger::load(Node::Object parent) -> void {
  memory.wram = parent->append<Node::Memory>("CD WRAM");
  memory.wram->setSize(64_KiB);
  memory.wram->setRead([&](uint32 address) -> uint8 {
    return pcd.wram.read(address);
  });
  memory.wram->setWrite([&](uint32 address, uint8 data) -> void {
    return pcd.wram.write(address, data);
  });

  memory.adpcm = parent->append<Node::Memory>("CD ADPCM");
  memory.adpcm->setSize(64_KiB);
  memory.adpcm->setRead([&](uint32 address) -> uint8 {
    return pcd.adpcm.memory.read(address);
  });
  memory.adpcm->setWrite([&](uint32 address, uint8 data) -> void {
    return pcd.adpcm.memory.write(address, data);
  });

  memory.bram = parent->append<Node::Memory>("CD BRAM");
  memory.bram->setSize(2_KiB);
  memory.bram->setRead([&](uint32 address) -> uint8 {
    return pcd.bram.memory.read(address);
  });
  memory.bram->setWrite([&](uint32 address, uint8 data) -> void {
    return pcd.bram.memory.write(address, data);
  });
}
