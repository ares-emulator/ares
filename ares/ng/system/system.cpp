#include <ng/ng.hpp>

namespace ares::NeoGeo {

auto enumerate() -> vector<string> {
  return {
    "[SNK] Neo Geo AES",
    "[SNK] Neo Geo MVS",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Scheduler scheduler;
System system;
#include "debugger.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.title();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("Neo Geo AES")) {
    information.name = "Neo Geo AES";
    information.model = Model::NeoGeoAES;
  }
  if(name.find("Neo Geo MVS")) {
    information.name = "Neo Geo MVS";
    information.model = Model::NeoGeoMVS;
  }

  node = Node::System::create(information.name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

  wram.allocate(64_KiB >> 1);
  if(NeoGeo::Model::NeoGeoMVS()) {
    sram.allocate(64_KiB >> 1);
  }

  scheduler.reset();
  cpu.load(node);
  apu.load(node);
  lspc.load(node);
  opnb.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  cardSlot.load(node);
  debugger.load(node);
  return true;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  debugger.unload(node);
  cpu.unload();
  apu.unload();
  lspc.unload();
  opnb.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  cardSlot.unload();
  wram.reset();
  sram.reset();
  pak.reset();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  cardSlot.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  if(auto fp = pak->read("bios.rom")) {
    bios.allocate(fp->size() >> 1);
    for(auto address : range(bios.size())) {
      bios.program(address, fp->readm(2L));
    }
  }

  if(NeoGeo::Model::NeoGeoMVS()) {
    if(auto fp = pak->read("static.rom")) {
      srom.allocate(fp->size() >> 1);
      for(auto address : range(srom.size())) {
        srom.program(address, fp->readl(2L));
      }
    }
  }

  if(cartridge.node) cartridge.power();
  cardSlot.power(reset);
  cpu.power(reset);
  apu.power(reset);
  lspc.power(reset);
  opnb.power(reset);
  scheduler.power(cpu);

  io = {};
}

};
