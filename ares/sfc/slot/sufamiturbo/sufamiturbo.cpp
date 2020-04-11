SufamiTurboCartridge& sufamiturboA = sufamiturboSlotA.cartridge;
SufamiTurboCartridge& sufamiturboB = sufamiturboSlotB.cartridge;
#include "slot.cpp"
#include "memory.cpp"
#include "serialization.cpp"

auto SufamiTurboCartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Sufami Turbo");
}

auto SufamiTurboCartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      fp->read(rom.data(), rom.size());
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(!(bool)memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Read)) {
        fp->read(ram.data(), ram.size());
      }
    }
  }
}

auto SufamiTurboCartridge::disconnect() -> void {
  if(!node) return;
  save();
  rom.reset();
  ram.reset();
  node = {};
}

auto SufamiTurboCartridge::power() -> void {
}

auto SufamiTurboCartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!(bool)memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Write)) {
        fp->write(ram.data(), ram.size());
      }
    }
  }
}
