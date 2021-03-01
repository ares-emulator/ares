SufamiTurboCartridge& sufamiturboA = sufamiturboSlotA.cartridge;
SufamiTurboCartridge& sufamiturboB = sufamiturboSlotB.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "serialization.cpp"

auto SufamiTurboCartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Sufami Turbo");
}

auto SufamiTurboCartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  if(auto fp = pak->read("manifest.bml")) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();

  if(auto memory = document["Game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = pak->read("program.rom")) {
      fp->read({rom.data(), rom.size()});
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(!(bool)memory["volatile"]) {
      if(auto fp = pak->read("save.ram")) {
        fp->read({ram.data(), ram.size()});
      }
    }
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
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!(bool)memory["volatile"]) {
      if(auto fp = pak->write("save.ram")) {
        fp->write({ram.data(), ram.size()});
      }
    }
  }
}
