#include <msx/msx.hpp>

namespace ares::MSX {

Scheduler scheduler;
ROM rom;
System system;
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) vdp.refresh();
}

auto System::load(Node::Object& root, Node::Object from) -> void {
  if(node) unload();

  information = {};
  if(interface->name() == "MSX" ) information.model = Model::MSX;
  if(interface->name() == "MSX2") information.model = Model::MSX2;

  node = Node::append<Node::System>(nullptr, from, interface->name());
  root = node;

  regionNode = Node::append<Node::String>(node, from, "Region", "NTSC → PAL");
  regionNode->setAllowedValues({
    "NTSC → PAL",
    "PAL → NTSC",
    "NTSC",
    "PAL"
  });

  scheduler.reset();
  keyboard.load(node, from);
  cpu.load(node, from);
  vdp.load(node, from);
  psg.load(node, from);
  cartridge.load(node, from);
  expansion.load(node, from);
  controllerPort1.load(node, from);
  controllerPort2.load(node, from);
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
  cartridge.unload();
  expansion.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  node = {};
  rom.bios.reset();
  rom.sub.reset();
}

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

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

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
