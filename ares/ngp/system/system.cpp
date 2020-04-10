#include <ngp/ngp.hpp>

namespace ares::NeoGeoPocket {

Scheduler scheduler;
System system;
#include "controls.cpp"
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) vpu.refresh();
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};
  if(interface->name() == "Neo Geo Pocket"      ) information.model = Model::NeoGeoPocket;
  if(interface->name() == "Neo Geo Pocket Color") information.model = Model::NeoGeoPocketColor;

  node = Node::System::create(interface->name());
  root = node;

  fastBoot = node->append<Node::Boolean>("Fast Boot", false);

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  apu.load(node);
  vpu.load(node);
  psg.load(node);
  cartridgeSlot.load(node);
}

auto System::save() -> void {
  if(!node) return;
  cpu.save();
  apu.save();
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  bios.reset();
  cpu.unload();
  apu.unload();
  vpu.unload();
  psg.unload();
  cartridgeSlot.unload();
  node = {};
}

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  bios.allocate(64_KiB);
  if(auto fp = platform->open(node, "bios.rom", File::Read, File::Required)) {
    bios.load(fp);
  }

  cartridge.power();
  cpu.power();
  apu.power();
  vpu.power();
  psg.power();
  scheduler.power(cpu);

  if(fastBoot->latch() && cartridge.flash[0]) cpu.fastBoot();

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);
}

}
