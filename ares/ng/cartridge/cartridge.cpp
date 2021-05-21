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

  if(auto fp = pak->read("voice.rom")) {
    vrom.allocate(fp->size());
    for(auto address : range(vrom.size())) {
      vrom.program(address, fp->read());
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
  vrom.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;
}

auto Cartridge::power() -> void {
}

}
