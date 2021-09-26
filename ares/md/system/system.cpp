#include <md/md.hpp>

namespace ares::MegaDrive {

auto enumerate() -> vector<string> {
  return {
    //Mega Drive
    "[Sega] Mega Drive (NTSC-J)",
    "[Sega] Mega Drive (NTSC-U)",
    "[Sega] Mega Drive (PAL)",
    //Mega 32X
    "[Sega] Mega 32X (NTSC-J)",
    "[Sega] Mega 32X (NTSC-U)",
    "[Sega] Mega 32X (PAL)",
    //Mega CD
    "[Sega] Mega CD (NTSC-J)",
    "[Sega] Mega CD (NTSC-U)",
    "[Sega] Mega CD (PAL)",
    //Mega CD 32X
    "[Sega] Mega CD 32X (NTSC-J)",
    "[Sega] Mega CD 32X (NTSC-U)",
    "[Sega] Mega CD 32X (PAL)",
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
  if(name.match("[Sega] Mega Drive (*)")) {
    information.name = "Mega Drive";
    information.mega32X = 0;
    information.megaCD = 0;
    cpu.minCyclesBetweenSyncs = 4; // sync approx every 7 pixels
  }
  if(name.match("[Sega] Mega 32X (*)")) {
    information.name = "Mega Drive";
    information.mega32X = 1;
    information.megaCD = 0;
    cpu.minCyclesBetweenSyncs = 15; // sync approx every 25-26 pixels
  }
  if(name.match("[Sega] Mega CD (*)")) {
    information.name = "Mega Drive";
    information.mega32X = 0;
    information.megaCD = 1;
    cpu.minCyclesBetweenSyncs = 4; // sync approx every 7 pixels
  }
  if(name.match("[Sega] Mega CD 32X (*)")) {
    information.name = "Mega Drive";
    information.mega32X = 1;
    information.megaCD = 1;
    cpu.minCyclesBetweenSyncs = 40; // sync approx every 70 pixels
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
  bus.load(node);
  cpu.load(node);
  apu.load(node);
  vdp.load(node);
  opn2.load(node);
  cartridgeSlot.load(node);
  if(Mega32X()) m32x.load(node);
  if(MegaCD()) mcd.load(node);
  controllerPort1.load(node);
  controllerPort2.load(node);
  extensionPort.load(node);
  return true;
}

auto System::unload() -> void {
  if(!node) return;
  save();
  bus.unload();
  cpu.unload();
  apu.unload();
  vdp.unload();
  opn2.unload();
  if(Mega32X()) m32x.unload();
  if(MegaCD()) mcd.unload();
  cartridgeSlot.unload();
  controllerPort1.unload();
  controllerPort2.unload();
  extensionPort.unload();
  pak.reset();
  node.reset();
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
  if(Mega32X()) m32x.save();
  if(MegaCD()) mcd.save();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  random.entropy(Random::Entropy::None);

  cartridge.power(reset);
  if(Mega32X()) m32x.power(reset);
  if(MegaCD()) mcd.power(reset);
  bus.power(reset);
  cpu.power(reset);
  apu.power(reset);  //apu.power() calls opn2.power()
  vdp.power(reset);  //vdp.power() calls vdp.psg.power()
  controllerPort1.power(reset);
  controllerPort2.power(reset);
  extensionPort.power(reset);
  scheduler.power(cpu);
}

}
