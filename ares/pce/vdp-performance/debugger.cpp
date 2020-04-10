auto VCE::Debugger::load(VCE& vce, Node::Object parent) -> void {
  memory.cram = parent->append<Node::Memory>("VCE CRAM");
  memory.cram->setSize(0x200 << 1);
  memory.cram->setRead([&](uint32 address) -> uint8 {
    return vce.cram.memory[uint9(address >> 1)].byte(address & 1);
  });
  memory.cram->setWrite([&](uint32 address, uint8 data) -> void {
    vce.cram.memory[uint9(address >> 1)].byte(address & 1) = data;
  });
}

auto VDC::Debugger::load(VDC& vdc, Node::Object parent) -> void {
  string vdcID = "VDC";
  if(Model::SuperGrafx()) vdcID = &vdc == &vdp.vdc0 ? "VDC0" : "VDC1";

  memory.vram = parent->append<Node::Memory>(string{vdcID, " VRAM"});
  memory.vram->setSize(32_KiB << 1);
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return vdc.vram.memory[uint15(address >> 1)].byte(address & 1);
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    vdc.vram.memory[uint15(address >> 1)].byte(address & 1) = data;
  });

  memory.satb = parent->append<Node::Memory>(string{vdcID, " SATB"});
  memory.satb->setSize(0x100 << 1);
  memory.satb->setRead([&](uint32 address) -> uint8 {
    return vdc.satb.memory[uint8(address >> 1)].byte(address & 1);
  });
  memory.satb->setWrite([&](uint32 address, uint8 data) -> void {
    vdc.satb.memory[uint8(address >> 1)].byte(address & 1) = data;
  });
}
