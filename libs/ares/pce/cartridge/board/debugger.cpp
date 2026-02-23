auto RAM::Debugger::load(Node::Object parent) -> void {
  if(super->ram) {
    memory.ram = parent->append<Node::Debugger::Memory>("HuCard RAM");
    memory.ram->setSize(super->ram.size());
    memory.ram->setRead([&](u32 address) -> u8 {
      return super->ram.read(address);
    });
    memory.ram->setWrite([&](u32 address, u8 data) -> void {
      return super->ram.write(address, data);
    });
  }
}

auto RAM::Debugger::unload(Node::Object parent) -> void {
  if(super->ram) {
    parent->remove(memory.ram);
    memory.ram = {};
  }
}

//

auto SuperSystemCard::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("HuCard RAM");
  memory.ram->setSize(super->ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return super->ram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return super->ram.write(address, data);
  });
}

auto SuperSystemCard::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.ram);
  memory.ram = {};
}

//

auto ArcadeCardDuo::Debugger::load(Node::Object parent) -> void {
  memory.dram = parent->append<Node::Debugger::Memory>("HuCard DRAM");
  memory.dram->setSize(super->dram.size());
  memory.dram->setRead([&](u32 address) -> u8 {
    return super->dram.read(address);
  });
  memory.dram->setWrite([&](u32 address, u8 data) -> void {
    return super->dram.write(address, data);
  });
}

auto ArcadeCardDuo::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.dram);
  memory.dram = {};
}

//

auto ArcadeCardPro::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("HuCard RAM");
  memory.ram->setSize(super->ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return super->ram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return super->ram.write(address, data);
  });

  memory.dram = parent->append<Node::Debugger::Memory>("HuCard DRAM");
  memory.dram->setSize(super->dram.size());
  memory.dram->setRead([&](u32 address) -> u8 {
    return super->dram.read(address);
  });
  memory.dram->setWrite([&](u32 address, u8 data) -> void {
    return super->dram.write(address, data);
  });
}

auto ArcadeCardPro::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.ram);
  parent->remove(memory.dram);
  memory.ram = {};
  memory.dram = {};
}
