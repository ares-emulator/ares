auto PI::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("PI RAM");
  memory.ram->setSize(64);
  memory.ram->setRead([&](u32 address) -> u8 {
    return pi.ram.readByte(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return pi.ram.writeByte(address, data);
  });

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "PI");
}

auto PI::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
