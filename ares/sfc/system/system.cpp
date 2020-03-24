#include <sfc/sfc.hpp>

namespace ares::SuperFamicom {

Random random;
Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) {
    ppu.refresh();

    auto reset = controls.reset->value();
    controls.poll();
    if(!reset && controls.reset->value()) power(true);
  }
}

auto System::load(Node::Object& root, Node::Object from) -> void {
  if(node) unload();

  information = {};

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
  bus.reset();
  controls.load(node, from);
  cpu.load(node, from);
  smp.load(node, from);
  dsp.load(node, from);
  ppu.load(node, from);
  cartridgeSlot.load(node, from);
  controllerPort1.load(node, from);
  controllerPort2.load(node, from);
  expansionPort.load(node, from);
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
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC") {
      information.region = Region::NTSC;
      information.cpuFrequency = Constants::Colorburst::NTSC * 6.0;
    }
    if(region == "PAL") {
      information.region = Region::PAL;
      information.cpuFrequency = Constants::Colorburst::PAL * 4.8;
    }
  };
  auto regionsHave = regionNode->latch().split("→").strip();
  setRegion(regionsHave.first());
  for(auto& have : reverse(regionsHave)) {
    if(have == cartridge.region()) setRegion(have);
  }

  random.entropy(Random::Entropy::Low);

  cpu.power(reset);
  smp.power(reset);
  dsp.power(reset);
  ppu.power(reset);
  cartridge.power(reset);
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
