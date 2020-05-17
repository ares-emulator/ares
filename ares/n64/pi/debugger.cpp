auto PI::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Memory>("PI RAM");
  memory.ram->setSize(64);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return pi.ram.readByte(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return pi.ram.writeByte(address, data);
  });
}
