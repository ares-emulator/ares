#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "flash.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  flash[0].reset(0);
  flash[1].reset(1);

  if(auto fp = pak->read("program.flash")) {
    auto size = fp->size();
    flash[0].allocate(min(16_Mibit, size));
    flash[1].allocate(size >= 16_Mibit ? size - 16_Mibit : 0);
    flash[0].load(fp);
    flash[1].load(fp);
  }

  debugger.load(node);
  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  debugger.unload(node);
  flash[0].reset(0);
  flash[1].reset(1);
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("program.flash")) {
    if(flash[0].modified || flash[1].modified) {
      flash[0].save(fp);
      flash[1].save(fp);
    }
  }
}

auto Cartridge::power() -> void {
  flash[0].power();
  flash[1].power();
}

auto Cartridge::read(n1 chip, n21 address) -> n8 {
  if(!flash[chip]) return 0x00;
  return flash[chip].read(address);
}

auto Cartridge::write(n1 chip, n21 address, n8 data) -> void {
  if(!flash[chip]) return;
  return flash[chip].write(address, data);
}

}
