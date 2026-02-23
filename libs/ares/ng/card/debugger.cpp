auto Card::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("Memory Card RAM");
  memory.ram->setSize(self.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return self.ram[address];
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    self.ram[address] = data;
  });
}

auto Card::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.ram);
  memory.ram.reset();
}
