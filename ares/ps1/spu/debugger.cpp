auto SPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("SPU RAM");
  memory.ram->setSize(spu.ram.size);
  memory.ram->setRead([&](u32 address) -> u8 {
    return spu.ram.readByte(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return spu.ram.writeByte(address, data);
  });
}
