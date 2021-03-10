#include <md/md.hpp>

namespace ares::MegaDrive {

auto enumerate() -> vector<string> {
  return {
    "[Sega] Mega Drive (NTSC-J)",
    "[Sega] Genesis (NTSC-U)",
    "[Sega] Mega Drive (PAL)",
    "[Sega] Mega CD (NTSC-J)",
    "[Sega] Sega CD (NTSC-U)",
    "[Sega] Mega CD (PAL)",
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
  if(mcd.node && (!cartridge.node || !cartridge.bootable())) {
    return mcd.title();
  }

  if(cartridge.node && cartridge.bootable()) {
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
  if(name.find("Mega Drive")) {
    information.name = "Mega Drive";
    information.megaCD = 0;
  }
  if(name.find("Genesis")) {
    information.name = "Mega Drive";
    information.megaCD = 0;
  }
  if(name.find("Mega CD")) {
    information.name = "Mega Drive";
    information.megaCD = 1;
  }
  if(name.find("Sega CD")) {
    information.name = "Mega Drive";
    information.megaCD = 1;
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
  if(!node->setPak(pak = platform->pak(node))) return false;

  tmss = node->append<Node::Setting::Boolean>("TMSS", false);

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  apu.load(node);
  vdp.load(node);
  psg.load(node);
  opn2.load(node);
  cartridgeSlot.load(node);
  expansionSlot.load(node);
  if(MegaCD()) mcd.load(node);
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
  opn2.unload();
  if(MegaCD()) mcd.unload();
  cartridgeSlot.unload();
  expansionSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  extensionPort.unload();
  pak.reset();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  if(MegaCD()) mcd.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  random.entropy(Random::Entropy::None);

  if(cartridge.node) cartridge.power();
  if(expansion.node) expansion.power();
  if(MegaCD()) mcd.power(reset);
  cpu.power(reset);
  apu.power(reset);
  vdp.power(reset);
  psg.power(reset);
  opn2.power(reset);
  controllerPort1.power();
  controllerPort2.power();
  extensionPort.power();
  scheduler.power(cpu);
}

}
