auto System::Debugger::load(Node::Object parent) -> void {
  memory.bios = parent->append<Node::Debugger::Memory>("BIOS");
  memory.bios->setSize(self.bios.size());
  memory.bios->setRead([&](u32 address) -> u8 {
    return self.bios.read(address);
  });
  memory.bios->setWrite([&](u32 address, u8 data) -> void {
    return self.bios.write(address, data);
  });
}

auto System::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.bios);
  memory.bios.reset();
}
