#include <msx/msx.hpp>

namespace ares::MSX {

auto load(Node::System& node, string name) -> bool {
  return system.load(node, name);
}

Scheduler scheduler;
ROM rom;
System system;
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node && expansion.node) {
    return {cartridge.name(), " + ", expansion.name()};
  }

  if(cartridge.node) {
    return cartridge.name();
  }

  if(expansion.node) {
    return expansion.name();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name == "MSX" ) information.model = Model::MSX;
  if(name == "MSX2") information.model = Model::MSX2;

  node = Node::System::create(name);
  node->setGame({&System::game, this});
  node->setRun({&System::run, this});
  node->setPower({&System::power, this});
  node->setSave({&System::save, this});
  node->setUnload({&System::unload, this});
  node->setSerialize({&System::serialize, this});
  node->setUnserialize({&System::unserialize, this});
  root = node;

  regionNode = node->append<Node::Setting::String>("Region", "NTSC → PAL");
  regionNode->setAllowedValues({
    "NTSC → PAL",
    "PAL → NTSC",
    "NTSC",
    "PAL"
  });

  scheduler.reset();
  keyboard.load(node);
  cpu.load(node);
  vdp.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
  expansionSlot.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  expansion.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  vdp.unload();
  psg.unload();
  cartridgeSlot.unload();
  expansionSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  node = {};
  rom.bios.reset();
  rom.sub.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC") {
      information.region = Region::NTSC;
      information.colorburst = Constants::Colorburst::NTSC;
    }
    if(region == "PAL") {
      information.region = Region::PAL;
      information.colorburst = Constants::Colorburst::PAL;
    }
  };
  auto regions = regionNode->latch().split("→").strip();
  setRegion(regions.first());
  for(auto& requested : reverse(regions)) {
    if(requested == cartridge.region()) setRegion(requested);
  }

  rom.bios.allocate(32_KiB);
  if(auto fp = platform->open(node, "bios.rom", File::Read, File::Required)) {
    rom.bios.load(fp);
  }

  if(model() == Model::MSX2) {
    rom.sub.allocate(16_KiB);
    if(auto fp = platform->open(node, "sub.rom", File::Read, File::Required)) {
      rom.sub.load(fp);
    }
  }

  keyboard.power();
  cartridge.power();
  expansion.power();
  cpu.power();
  vdp.power();
  psg.power();
  scheduler.power(cpu);
}

}
