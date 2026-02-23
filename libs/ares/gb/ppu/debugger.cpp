auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("PPU VRAM");
  memory.vram->setSize(ppu.vram.size());
  memory.vram->setRead([&](u32 address) -> u8 {
    return ppu.vram[address];
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    ppu.vram[address] = data;
  });

  memory.oam = parent->append<Node::Debugger::Memory>("PPU OAM");
  memory.oam->setSize(ppu.oam.size());
  memory.oam->setRead([&](u32 address) -> u8 {
    return ppu.oam[address];
  });
  memory.oam->setWrite([&](u32 address, u8 data) -> void {
    ppu.oam[address] = data;
  });

  memory.bgp = parent->append<Node::Debugger::Memory>("PPU BGP");
  memory.bgp->setSize(ppu.bgp.size());
  memory.bgp->setRead([&](u32 address) -> u8 {
    return ppu.bgp[address];
  });
  memory.bgp->setWrite([&](u32 address, u8 data) -> void {
    ppu.bgp[address] = data;
  });

  memory.obp = parent->append<Node::Debugger::Memory>("PPU OBP");
  memory.obp->setSize(ppu.obp.size());
  memory.obp->setRead([&](u32 address) -> u8 {
    return ppu.obp[address];
  });
  memory.obp->setWrite([&](u32 address, u8 data) -> void {
    ppu.obp[address] = data;
  });

  if(!Model::GameBoyColor()) return;

  memory.bgpd = parent->append<Node::Debugger::Memory>("PPU BGPD");
  memory.bgpd->setSize(ppu.bgpd.size() << 1);
  memory.bgpd->setRead([&](u32 address) -> u8 {
    return ppu.bgpd[address >> 1].byte(address & 1);
  });
  memory.bgpd->setWrite([&](u32 address, u8 data) -> void {
    ppu.bgpd[address >> 1].byte(address & 1) = data;
  });

  memory.obpd = parent->append<Node::Debugger::Memory>("PPU OBPD");
  memory.obpd->setSize(ppu.obpd.size() << 1);
  memory.obpd->setRead([&](u32 address) -> u8 {
    return ppu.obpd[address >> 1].byte(address & 1);
  });
  memory.obpd->setWrite([&](u32 address, u8 data) -> void {
    ppu.obpd[address >> 1].byte(address & 1) = data;
  });
}
