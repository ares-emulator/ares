auto DSP::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("APU RAM");
  memory.ram->setSize(64_KiB);
  memory.ram->setRead([&](u32 address) -> u8 {
    return dsp.apuram[n16(address)];
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    dsp.apuram[n16(address)] = data;
  });
}
