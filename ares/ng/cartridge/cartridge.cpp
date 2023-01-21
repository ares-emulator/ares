#include <ng/ng.hpp>

namespace ares::NeoGeo {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>("Neo Geo Cartridge");
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  if(auto fp = pak->read("program.rom")) {
    prom.allocate(fp->size() >> 1);
    for(auto address : range(prom.size())) {
      prom.program(address, fp->readm(2L));
    }
  }

  if(auto fp = pak->read("music.rom")) {
    mrom.allocate(fp->size());
    for(auto address : range(mrom.size())) {
      mrom.program(address, fp->read());
    }
  }

  if(auto fp = pak->read("character.rom")) {
    crom.allocate(fp->size());
    for(auto address : range(crom.size())) {
      crom.program(address, fp->read());
    }
  }

  if(auto fp = pak->read("static.rom")) {
    srom.allocate(fp->size());
    for(auto address : range(srom.size())) {
      srom.program(address, fp->read());
    }
  }

  if(auto fp = pak->read("voice-a.rom")) {
    vromA.allocate(fp->size());
    for(auto address : range(vromA.size())) {
      vromA.program(address, fp->read());
    }
  }

  if(auto fp = pak->read("voice-b.rom")) {
    vromB.allocate(fp->size());
    for(auto address : range(vromB.size())) {
      vromB.program(address, fp->read());
    }
  }

  debugger.load(node);
  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  debugger.unload(node);
  prom.reset();
  mrom.reset();
  crom.reset();
  srom.reset();
  vromA.reset();
  vromB.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::readProgram(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  if(address <= 0x0fffff) return prom[address >> 1];
  if(address >= 0x200000 && address <= 0x2fffff) {
    address = ((bank + 1) * 0x100000) | n20(address);
    return prom[address >> 1];
  }

  return data;
}

auto Cartridge::writeProgram(n1 upper, n1 lower, n24 address, n16 data) -> void {
  if(lower && address >= 0x200000 && address <= 0x2fffff) {
    bank = data.bit(0, 2);
  }
}


auto Cartridge::save() -> void {
  if(!node) return;
}

auto Cartridge::power() -> void {
  bank = 0;
}

}
