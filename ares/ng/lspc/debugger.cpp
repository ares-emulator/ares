auto LSPC::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("LSPC Video RAM");
  memory.vram->setSize(lspc.vram.size() << 1);
  memory.vram->setRead([&](u32 address) -> u8 {
    return lspc.vram[address >> 1].byte(!(address & 1));
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    lspc.vram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.pram = parent->append<Node::Debugger::Memory>("LSPC Palette RAM");
  memory.pram->setSize(lspc.pram.size() << 1);
  memory.pram->setRead([&](u32 address) -> u8 {
    return lspc.pram[address >> 1].byte(!(address & 1));
  });
  memory.pram->setWrite([&](u32 address, u8 data) -> void {
    lspc.pram[address >> 1].byte(!(address & 1)) = data;
  });
}

auto LSPC::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.vram);
  parent->remove(memory.pram);
  memory.vram.reset();
  memory.pram.reset();
}
