#include <md/md.hpp>

namespace ares::MegaDrive {

Expansion& expansion = expansionPort.expansion;
#include "port.cpp"
#include "serialization.cpp"

auto Expansion::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(interface->name());
}

auto Expansion::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();
  information.regions = document["game/region"].text().split(",").strip();

  mcd.load(node);

  power();
}

auto Expansion::disconnect() -> void {
  if(!node) return;
  node = {};
}

auto Expansion::save() -> void {
  if(!node) return;
}

auto Expansion::power() -> void {
}

/* the only existing expansion port device is the Mega CD, which is hard-coded below for now */

auto Expansion::read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
  return mcd.external_read(upper, lower, address, data);
}

auto Expansion::write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
  return mcd.external_write(upper, lower, address, data);
}

auto Expansion::readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16 {
  if(!node) return data;
  return mcd.external_readIO(upper, lower, address, data);
}

auto Expansion::writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void {
  if(!node) return;
  return mcd.external_writeIO(upper, lower, address, data);
}

}
