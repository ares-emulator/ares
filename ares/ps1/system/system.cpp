#include <ps1/ps1.hpp>

namespace ares::PlayStation {

auto enumerate() -> vector<string> {
  return {
    "[Sony] PlayStation (NTSC-J)",
    "[Sony] PlayStation (NTSC-U)",
    "[Sony] PlayStation (PAL)",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

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
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  if constexpr(Accuracy::CPU::Recompiler) {
    ares::Memory::FixedAllocator::get().release();
  }
  bios.setWaitStates(6, 12, 24);
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
