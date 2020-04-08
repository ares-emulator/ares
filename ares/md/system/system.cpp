#include <md/md.hpp>

namespace ares::MegaDrive {

Random random;
Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) vdp.refresh();

  auto reset = controls.reset->value();
  controls.poll();
  if(!reset && controls.reset->value()) power(true);
}

auto System::load(Node::Object& root, Node::Object from) -> void {
  if(node) unload();

  information = {};

  node = Node::append<Node::System>(nullptr, from, interface->name());
  root = node;

  tmss = Node::append<Node::Boolean>(node, from, "TMSS", false);

  regionNode = Node::append<Node::String>(node, from, "Region", "NTSC-J → NTSC-U → PAL");
  regionNode->setAllowedValues({
    "NTSC-J → NTSC-U → PAL",
    "NTSC-U → NTSC-J → PAL",
    "PAL → NTSC-J → NTSC-U",
    "PAL → NTSC-U → NTSC-J",
    "NTSC-J",
    "NTSC-U",
    "PAL"
  });

  scheduler.reset();
  controls.load(node, from);
  cpu.load(node, from);
  apu.load(node, from);
  vdp.load(node, from);
  psg.load(node, from);
  ym2612.load(node, from);
  cartridgeSlot.load(node, from);
  expansionPort.load(node, from);
  controllerPort1.load(node, from);
  controllerPort2.load(node, from);
  extensionPort.load(node, from);
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  apu.unload();
  vdp.unload();
  psg.unload();
  ym2612.unload();
  cartridgeSlot.unload();
  expansionPort.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  extensionPort.unload();
  mcd.unload();
  node = {};
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  expansion.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  auto setRegion = [&](string region) {
    if(region == "NTSC-J") {
      information.region = Region::NTSCJ;
      information.frequency = Constants::Colorburst::NTSC * 15.0;
    }
    if(region == "NTSC-U") {
      information.region = Region::NTSCU;
      information.frequency = Constants::Colorburst::NTSC * 15.0;
    }
    if(region == "PAL") {
      information.region = Region::PAL;
      information.frequency = Constants::Colorburst::PAL * 12.0;
    }
  };
  auto regionsHave = regionNode->latch().split("→").strip();
  auto regionsWant = cartridge.bootable() ? cartridge.regions() : expansion.regions();
  setRegion(regionsHave.first());
  for(auto& have : reverse(regionsHave)) {
    for(auto& want : reverse(regionsWant)) {
      if(have == want) setRegion(have);
    }
  }
  information.megaCD = (bool)expansion.node;

  random.entropy(Random::Entropy::Low);

  if(cartridge.node) cartridge.power();
  if(expansion.node) expansion.power();
  cpu.power(reset);
  apu.power(reset);
  vdp.power(reset);
  psg.power(reset);
  ym2612.power(reset);
  if(MegaCD()) mcd.power(reset);
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
