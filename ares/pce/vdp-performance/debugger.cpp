auto VCE::Debugger::load(VCE& vce, Node::Object parent) -> void {
  memory.cram = parent->append<Node::Debugger::Memory>("VCE CRAM");
  memory.cram->setSize(0x200 << 1);
  memory.cram->setRead([&](u32 address) -> u8 {
    return vce.cram.memory[n9(address >> 1)].byte(address & 1);
  });
  memory.cram->setWrite([&](u32 address, u8 data) -> void {
    vce.cram.memory[n9(address >> 1)].byte(address & 1) = data;
  });
}

auto VDC::Debugger::load(VDC& vdc, Node::Object parent) -> void {
  string vdcID = "VDC";
  if(Model::SuperGrafx()) vdcID = &vdc == &vdp.vdc0 ? "VDC0" : "VDC1";

  memory.vram = parent->append<Node::Debugger::Memory>(string{vdcID, " VRAM"});
  memory.vram->setSize(32_KiB << 1);
  memory.vram->setRead([&](u32 address) -> u8 {
    return vdc.vram.memory[n15(address >> 1)].byte(address & 1);
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    vdc.vram.memory[n15(address >> 1)].byte(address & 1) = data;
  });

  memory.satb = parent->append<Node::Debugger::Memory>(string{vdcID, " SATB"});
  memory.satb->setSize(0x100 << 1);
  memory.satb->setRead([&](u32 address) -> u8 {
    return vdc.satb.memory[n8(address >> 1)].byte(address & 1);
  });
  memory.satb->setWrite([&](u32 address, u8 data) -> void {
    vdc.satb.memory[n8(address >> 1)].byte(address & 1) = data;
  });
}
