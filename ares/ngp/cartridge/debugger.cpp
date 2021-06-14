auto Cartridge::Debugger::load(Node::Object parent) -> void {
  memory.rom = parent->append<Node::Debugger::Memory>("Cartridge Flash ROM");
  memory.rom->setSize(self.flash[0].rom.size() + self.flash[1].rom.size());
  memory.rom->setRead([&](u32 address) -> u8 {
    bool select = address >= self.flash[0].rom.size();
    return self.flash[select].rom.read(address);
  });
  memory.rom->setWrite([&](u32 address, u8 data) -> void {
    bool select = address >= self.flash[0].rom.size();
    return self.flash[select].rom.write(address, data);
  });
}

auto Cartridge::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.rom);
  memory.rom.reset();
}
