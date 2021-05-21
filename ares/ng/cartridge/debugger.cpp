auto Cartridge::Debugger::load(Node::Object parent) -> void {
  memory.prom = parent->append<Node::Debugger::Memory>("Cartridge Program ROM");
  memory.prom->setSize(cartridge.prom.size() << 1);
  memory.prom->setRead([&](u32 address) -> u8 {
    return cartridge.prom[address >> 1].byte(!(address & 1));
  });
  memory.prom->setWrite([&](u32 address, u8 data) -> void {
    cartridge.prom[address >> 1].byte(!(address & 1)) = data;
  });

  memory.mrom = parent->append<Node::Debugger::Memory>("Cartridge Music ROM");
  memory.mrom->setSize(cartridge.mrom.size());
  memory.mrom->setRead([&](u32 address) -> u8 {
    return cartridge.mrom[address];
  });
  memory.mrom->setWrite([&](u32 address, u8 data) -> void {
    cartridge.mrom[address] = data;
  });

  memory.crom = parent->append<Node::Debugger::Memory>("Cartridge Character ROM");
  memory.crom->setSize(cartridge.crom.size());
  memory.crom->setRead([&](u32 address) -> u8 {
    return cartridge.crom[address];
  });
  memory.crom->setWrite([&](u32 address, u8 data) -> void {
    cartridge.crom[address] = data;
  });

  memory.srom = parent->append<Node::Debugger::Memory>("Cartridge Sprite ROM");
  memory.srom->setSize(cartridge.srom.size());
  memory.srom->setRead([&](u32 address) -> u8 {
    return cartridge.srom[address];
  });
  memory.srom->setWrite([&](u32 address, u8 data) -> void {
    cartridge.srom[address] = data;
  });

  memory.vrom = parent->append<Node::Debugger::Memory>("Cartridge Voice ROM");
  memory.vrom->setSize(cartridge.vrom.size());
  memory.vrom->setRead([&](u32 address) -> u8 {
    return cartridge.vrom[address];
  });
  memory.vrom->setWrite([&](u32 address, u8 data) -> void {
    cartridge.vrom[address] = data;
  });
}

auto Cartridge::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.prom);
  parent->remove(memory.mrom);
  parent->remove(memory.crom);
  parent->remove(memory.srom);
  parent->remove(memory.vrom);
  memory.prom.reset();
  memory.mrom.reset();
  memory.crom.reset();
  memory.srom.reset();
  memory.vrom.reset();
}
