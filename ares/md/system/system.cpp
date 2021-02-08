#include <md/md.hpp>

namespace ares::MegaDrive {

auto enumerate() -> vector<string> {
  return {
    "[Sega] Mega Drive (NTSC-J)",
    "[Sega] Genesis (NTSC-U)",
    "[Sega] Mega Drive (PAL)",
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
  if(expansion.node && (!cartridge.node || !cartridge.bootable())) {
    if(mcd.disc) return mcd.name();
    return expansion.name();
  }

  if(cartridge.node && cartridge.bootable()) {
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
  if(name.find("Mega Drive")) {
    information.name = "Mega Drive";
  }
  if(name.find("Genesis")) {
    information.name = "Mega Drive";
  }
  if(name.find("NTSC-J")) {
    information.region = Region::NTSCJ;
    information.frequency = Constants::Colorburst::NTSC * 15.0;
  }
  if(name.find("NTSC-U")) {
    information.region = Region::NTSCU;
    information.frequency = Constants::Colorburst::NTSC * 15.0;
  }
  if(name.find("PAL")) {
    information.region = Region::PAL;
    information.frequency = Constants::Colorburst::PAL * 12.0;
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

  tmss = node->append<Node::Setting::Boolean>("TMSS", false);

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  apu.load(node);
  vdp.load(node);
  psg.load(node);
  ym2612.load(node);
  cartridgeSlot.load(node);
  expansionPort.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  extensionPort.load(node);
  return true;
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
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

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
}

}
