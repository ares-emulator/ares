auto RDRAM::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Notification>("I/O", "RDRAM");

  memory.ram = parent->append<Node::Memory>("RDRAM");
  memory.ram->setSize(4_MiB + 4_MiB);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return rdram.ram.readByte(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return rdram.ram.writeByte(address, data);
  });
}

auto RDRAM::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
