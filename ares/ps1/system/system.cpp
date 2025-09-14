#include <ps1/ps1.hpp>
#include <algorithm>

namespace ares::PlayStation {

auto enumerate() -> std::vector<string> {
  return {
    "[Sony] PlayStation (NTSC-J)",
    "[Sony] PlayStation (NTSC-U)",
    "[Sony] PlayStation (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  auto list = enumerate();
  if(std::find(list.begin(), list.end(), name) == list.end()) return false;
  return system.load(node, name);
}

auto option(string name, string value) -> bool {
  if(name == "Homebrew Mode") system.homebrewMode = value.boolean();
  return true;
}

Random random;
System system;
#include "serialization.cpp"

auto System::game() -> string {
  if(disc.cd) {
    return disc.title();
  }

  return "(no disc inserted)";
}

auto System::run() -> void {
  while(!gpu.refreshed) cpu.main();
  gpu.refreshed = false;
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("PlayStation")) {
    information.name = "PlayStation";
  }
  if(name.find("NTSC-J")) {
    information.region = Region::NTSCJ;
  }
  if(name.find("NTSC-U")) {
    information.region = Region::NTSCU;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
  }

  node = Node::System::create(information.name);
  node->setAttribute("configuration", name);
  node->setGame(std::bind_front(&System::game, this));
  node->setRun(std::bind_front(&System::run, this));
  node->setPower(std::bind_front(&System::power, this));
  node->setSave(std::bind_front(&System::save, this));
  node->setUnload(std::bind_front(&System::unload, this));
  node->setSerialize([this](bool save) -> serializer { return serialize(save); });
  node->setUnserialize(std::bind_front(&System::unserialize, this));
  root = node;
  if(!node->setPak(pak = platform->pak(node))) return false;

  fastBoot = node->append<Node::Setting::Boolean>("Fast Boot", false);

  memory.load(node);
  cpu.load(node);
  gpu.load(node);
  spu.load(node);
  mdec.load(node);
  disc.load(node);
  controllerPort1.load(node);
  memoryCardPort1.load(node);
  controllerPort2.load(node);
  memoryCardPort2.load(node);
  interrupt.load(node);
  peripheral.load(node);
  dma.load(node);
  timer.load(node);

  bios.allocate(512_KiB);
  if(auto fp = pak->read("bios.rom")) {
    bios.load(fp);
  }

  return true;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  if(gpu.screen) gpu.screen->quit(); //stop video thread
  memory.unload();
  cpu.unload();
  gpu.unload();
  spu.unload();
  mdec.unload();
  disc.unload();
  controllerPort1.unload();
  memoryCardPort1.unload();
  controllerPort2.unload();
  memoryCardPort2.unload();
  interrupt.unload();
  peripheral.unload();
  dma.unload();
  timer.unload();
  fastBoot.reset();
  pak.reset();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  memoryCardPort1.save();
  memoryCardPort2.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  random.entropy(Random::Entropy::High);
  if(system.homebrewMode) {
    random.seed(Random::Default);
  }

  bios.setWaitStates(8, 16, 31);
  memory.power(reset);
  cpu.power(reset);
  gpu.power(reset);
  spu.power(reset);
  mdec.power(reset);
  disc.power(reset);
  interrupt.power(reset);
  peripheral.power(reset);
  dma.power(reset);
  timer.power(reset);
}

}
