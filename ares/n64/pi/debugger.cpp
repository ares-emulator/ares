auto PI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Notification>("I/O", "PI");

  memory.ram = parent->append<Node::Memory>("PI RAM");
  memory.ram->setSize(64);
  memory.ram->setRead([&](uint32 address) -> uint8 {
    return pi.ram.readByte(address);
  });
  memory.ram->setWrite([&](uint32 address, uint8 data) -> void {
    return pi.ram.writeByte(address, data);
  });
}

auto PI::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
