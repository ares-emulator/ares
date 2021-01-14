auto RDRAM::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("RDRAM");
  memory.ram->setSize(4_MiB + 4_MiB);
  memory.ram->setRead([&](u32 address) -> u8 {
    return rdram.ram.readByte(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return rdram.ram.writeByte(address, data);
  });

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "RDRAM");
}

auto RDRAM::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
