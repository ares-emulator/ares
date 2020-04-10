auto DSP::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("APU RAM");
  memory.ram->setSize(64_KiB);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return dsp.apuram[uint16(address)];
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    dsp.apuram[uint16(address)] = data;
  });
}
