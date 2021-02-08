#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

auto enumerate() -> vector<string> {
  //todo: lucia can't distinguish "Super Famicom (NTSC)" from "Super Nintendo (NTSC)"
  return {
    "[Nintendo] Super Famicom (NTSC)",
    "[Nintendo] Super Famicom (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Random random;
Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  #if defined(CORE_GB)
  if(cartridge.has.GameBoySlot && GameBoy::cartridge.node) {
    return GameBoy::cartridge.name();
  }
  #endif

  if(bsmemory.node) {
    return {cartridge.name(), " + ", bsmemory.name()};
  }

  if(sufamiturboA.node && sufamiturboB.node) {
    return {sufamiturboA.name(), " + ", sufamiturboB.name()};
  }

  if(sufamiturboA.node) {
    return sufamiturboA.name();
  }

  if(sufamiturboB.node) {
    return sufamiturboB.name();
  }

  if(cartridge.node) {
    return cartridge.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
  auto reset = controls.reset->value();
  controls.poll();
  if(!reset && controls.reset->value()) power(true);
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("Super Famicom")) {
    information.name = "Super Famicom";
  }
  if(name.find("Super Nintendo")) {
    information.name = "Super Nintendo";
  }
  if(name.find("NTSC")) {
    information.region = Region::NTSC;
    information.cpuFrequency = Constants::Colorburst::NTSC * 6.0;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.cpuFrequency = Constants::Colorburst::PAL * 4.8;
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

  scheduler.reset();
  bus.reset();
  controls.load(node);
  cpu.load(node);
  smp.load(node);
  dsp.load(node);
  ppu.load(node);
  cartridgeSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  expansionPort.load(node);
  return true;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  smp.unload();
  dsp.unload();
  ppu.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  expansionPort.unload();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  random.entropy(Random::Entropy::Low);

  cpu.power(reset);
  smp.power(reset);
  dsp.power(reset);
  ppu.power(reset);
  cartridge.power(reset);
  scheduler.power(cpu);
}

}
