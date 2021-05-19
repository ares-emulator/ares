auto GPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("GPU Video RAM");
  memory.vram->setSize(gpu.vram.size() << 1);
  memory.vram->setRead([&](u32 address) -> u8 {
    return gpu.vram[address >> 1].byte(!(address & 1));
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    gpu.vram[address >> 1].byte(!(address & 1)) = data;
  });

  memory.pram = parent->append<Node::Debugger::Memory>("GPU Palette RAM");
  memory.pram->setSize(gpu.pram.size() << 1);
  memory.pram->setRead([&](u32 address) -> u8 {
    return gpu.pram[address >> 1].byte(!(address & 1));
  });
  memory.pram->setWrite([&](u32 address, u8 data) -> void {
    gpu.pram[address >> 1].byte(!(address & 1)) = data;
  });
}

auto GPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.vram);
  parent->remove(memory.pram);
  memory.vram.reset();
  memory.pram.reset();
}
