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
  if(!node->setPak(pak = platform->pak(node))) return false;

  fastBoot = node->append<Node::Setting::Boolean>("Fast Boot", false);

  bios.allocate(64_KiB);
  if(auto fp = pak->read("bios.rom")) {
    bios.load(fp);
  }

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  apu.load(node);
  kge.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
  debugger.load(node);
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
  debugger.unload(node);
  bios.reset();
  cpu.unload();
  apu.unload();
  kge.unload();
  psg.unload();
  cartridgeSlot.unload();
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  //enable or disable boot animation.
  n16 bootAnim = NeoGeoPocket::Model::NeoGeoPocketColor() ? 0x530c : 0x4618;
  bios.program(bootAnim, fastBoot->latch() ? 0x0e : 0xf1); //0x0e = ret, 0xf1 = ld

  cartridge.power();
  cpu.power();
  apu.power();
  kge.power();
  psg.power();
  scheduler.power(cpu);
}

}
