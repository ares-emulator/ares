#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

auto enumerate() -> vector<string> {
  return {
    "[SNK] Neo Geo Pocket",
    "[SNK] Neo Geo Pocket Color",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("Neo Geo Pocket")) {
    information.name = "Neo Geo Pocket";
    information.model = Model::NeoGeoPocket;
  }
  if(name.find("Neo Geo Pocket Color")) {
    information.name = "Neo Geo Pocket Color";
    information.model = Model::NeoGeoPocketColor;
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

  fastBoot = node->append<Node::Setting::Boolean>("Fast Boot", false);

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  apu.load(node);
  vpu.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cpu.save();
  apu.save();
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  bios.reset();
  cpu.unload();
  apu.unload();
  vpu.unload();
  psg.unload();
  cartridgeSlot.unload();
  node = {};
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  bios.allocate(64_KiB);
  if(auto fp = platform->open(node, "bios.rom", File::Read, File::Required)) {
    bios.load(fp);
  }

  cartridge.power();
  cpu.power();
  apu.power();
  vpu.power();
  psg.power();
  scheduler.power(cpu);

  if(fastBoot->latch() && cartridge.flash[0]) cpu.fastBoot();
}

}
