auto SPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("SPU RAM");
  memory.ram->setSize(spu.ram.size);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return spu.ram.readByte(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return spu.ram.writeByte(address, data);
  });
}
