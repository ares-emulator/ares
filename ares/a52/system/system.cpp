#include <a52/a52.hpp>
#include <algorithm>

namespace ares::Atari5200 {

auto enumerate() -> std::vector<string> {
  return {
    "[Atari] Atari 5200 (NTSC)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto configurations = enumerate();
  if(std::find(configurations.begin(), configurations.end(), name) == configurations.end()) return false;
  return system.load(node, name);
}

Random random;
Scheduler scheduler;
System system;

auto System::game() -> string {
  if(cartridge.node) return cartridge.title();
  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
  for(auto& port : controllerPorts) port.poll();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  node = std::make_shared<Core::System>(information.name);
  node->setAttribute("configuration", name);
  node->setGame(std::bind_front(&System::game, this));
  node->setRun(std::bind_front(&System::run, this));
  node->setPower(std::bind_front(&System::power, this));
  node->setSave(std::bind_front(&System::save, this));
  node->setUnload(std::bind_front(&System::unload, this));
  root = node;

  if(!node->setPak(pak = platform->pak(node))) {
    root = {};
    node = {};
    return false;
  }

  auto firmware = pak->read("bios.rom");
  if(!firmware || firmware->size() != 2_KiB) {
    pak.reset();
    root = {};
    node = {};
    return false;
  }

  ram.allocate(16_KiB);
  bios.allocate(2_KiB);
  bios.load(firmware);

  scheduler.reset();
  cpu.load(node);
  antic.load(node);
  gtia.load(node);
  pokey.load(node);
  cartridgeSlot.load(node);
  for(auto& port : controllerPorts) port.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  antic.unload();
  gtia.unload();
  pokey.unload();
  cartridgeSlot.unload();
  for(auto& port : controllerPorts) port.unload();
  ram.reset();
  bios.reset();
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  random.entropy(Random::Entropy::Low);
  if(!reset) {
    for(auto& byte : ram) byte = random();
  }

  cartridge.power();
  antic.power();
  gtia.power();
  pokey.power();
  cpu.power(reset);
  scheduler.power(cpu);
}

}
