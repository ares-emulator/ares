#include <sfc/sfc.hpp>
#include <algorithm>

namespace ares::SuperFamicom {

auto enumerate() -> std::vector<string> {
  return {
    "[Nintendo] Super Famicom (NTSC)",
    "[Nintendo] Super Famicom (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

auto option(string name, string value) -> bool {
  if(name == "Pixel Accuracy") ppu.setAccurate(value.boolean());
  return true;
}

Random random;
Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  #if defined(CORE_GB)
  if(cartridge.has.GameBoySlot && GameBoy::cartridge.node) {
    return GameBoy::cartridge.title();
  }
  #endif

  if(bsmemory.node) {
    return {cartridge.title(), " + ", bsmemory.title()};
  }

  if(sufamiturboA.node && sufamiturboB.node) {
    return {sufamiturboA.title(), " + ", sufamiturboB.title()};
  }

  if(sufamiturboA.node) {
    return sufamiturboA.title();
  }

  if(sufamiturboB.node) {
    return sufamiturboB.title();
  }

  if(cartridge.node) {
    return cartridge.title();
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
  if(name.find("NTSC")) {
    information.region = Region::NTSC;
    information.cpuFrequency = Constants::Colorburst::NTSC * 6.0;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.cpuFrequency = Constants::Colorburst::PAL * 4.8;
  }

  node = Node::System::create(information.name);
  node->setAttribute("configuration", name);
  node->setGame(std::bind_front(&System::game, this));
  node->setRun(std::bind_front(&System::run, this));
  node->setPower(std::bind_front(&System::power, this));
  node->setSave(std::bind_front(&System::save, this));
  node->setUnload(std::bind_front(&System::unload, this));
  node->setSerialize([this](bool s){ return this->serialize(s); });
  node->setUnserialize([this](serializer& s){ return this->unserialize(s); });
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

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
  pak.reset();
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
