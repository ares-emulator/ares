SufamiTurboCartridge& sufamiturboA = sufamiturboSlotA.cartridge;
SufamiTurboCartridge& sufamiturboB = sufamiturboSlotB.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "serialization.cpp"

auto SufamiTurboCartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Sufami Turbo Cartridge");
}

auto SufamiTurboCartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;
  information = {};
  information.title = pak->attribute("title");

  if(auto fp = pak->read("program.rom")) {
    rom.allocate(fp->size());
    fp->read({rom.data(), rom.size()});
  }

  if(auto fp = pak->read("save.ram")) {
    ram.allocate(fp->size());
    fp->read({ram.data(), ram.size()});
  }
}

auto SufamiTurboCartridge::disconnect() -> void {
  if(!node) return;
  save();
  rom.reset();
  ram.reset();
  pak.reset();
  node.reset();
}

auto SufamiTurboCartridge::power() -> void {
}

auto SufamiTurboCartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("save.ram")) {
    fp->write({ram.data(), ram.size()});
  }
}
