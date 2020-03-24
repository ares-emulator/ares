#include <gba/gba.hpp>

namespace ares::GameBoyAdvance {

Scheduler scheduler;
System system;
#include "bios.cpp"
#include "controls.cpp"
#include "serialization.cpp"

auto System::run() -> void {
  if(scheduler.enter() == Event::Frame) ppu.refresh();
  if(GameBoyAdvance::Model::GameBoyPlayer()) player.frame();
}

auto System::load(Node::Object& root, Node::Object from) -> void {
  if(node) unload();

  information = {};
  if(interface->name() == "Game Boy Advance") information.model = Model::GameBoyAdvance;
  if(interface->name() == "Game Boy Player" ) information.model = Model::GameBoyPlayer;

  node = Node::append<Node::System>(nullptr, from, interface->name());
  root = node;

  scheduler.reset();
  controls.load(node, from);
  cpu.load(node, from);
  ppu.load(node, from);
  apu.load(node, from);
  cartridge.load(node, from);
}

auto System::save() -> void {
  if(!node) return;
  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;
  save();
  cpu.unload();
  ppu.unload();
  apu.unload();
  cartridge.unload();
  node = {};
}

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  if(auto fp = platform->open(node, "bios.rom", File::Read, File::Required)) {
    fp->read(bios.data, bios.size);
  }

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);

  bus.power();
  player.power();
  cpu.power();
  ppu.power();
  apu.power();
  cartridge.power();
  scheduler.power(cpu);
}

}
