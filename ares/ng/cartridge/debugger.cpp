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

  memory.vromA = parent->append<Node::Debugger::Memory>("Cartridge Voice ROM (A)");
  memory.vromA->setSize(cartridge.vromA.size());
  memory.vromA->setRead([&](u32 address) -> u8 {
    return cartridge.vromA[address];
  });
  memory.vromA->setWrite([&](u32 address, u8 data) -> void {
    cartridge.vromA[address] = data;
  });

  memory.vromB = parent->append<Node::Debugger::Memory>("Cartridge Voice ROM (B)");
  memory.vromB->setSize(cartridge.vromB.size());
  memory.vromB->setRead([&](u32 address) -> u8 {
    return cartridge.vromB[address];
  });
  memory.vromB->setWrite([&](u32 address, u8 data) -> void {
    cartridge.vromB[address] = data;
  });
}

auto Cartridge::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.prom);
  parent->remove(memory.mrom);
  parent->remove(memory.crom);
  parent->remove(memory.srom);
  parent->remove(memory.vromA);
  parent->remove(memory.vromB);
  memory.prom.reset();
  memory.mrom.reset();
  memory.crom.reset();
  memory.srom.reset();
  memory.vromA.reset();
  memory.vromB.reset();
}
