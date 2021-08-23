#include <gb/gb.hpp>

namespace ares::GameBoy {

auto enumerate() -> vector<string> {
  return {
    "[Nintendo] Game Boy",
    "[Nintendo] Game Boy Color",
    "[Nintendo] Super Game Boy",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Scheduler scheduler;
System system;
SuperGameBoyInterface* superGameBoy = nullptr;
#include "controls.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.title();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
}

auto System::clocksExecuted() -> u32 {
  u32 clocks = information.clocksExecuted;
  information.clocksExecuted = 0;
  return clocks;
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("Game Boy")) {
    information.name = "Game Boy";
    information.model = Model::GameBoy;
  }
  if(name.find("Game Boy Color")) {
    information.name = "Game Boy Color";
    information.model = Model::GameBoyColor;
  }
  if(name.find("Super Game Boy")) {
    information.name = "Super Game Boy";
    information.model = Model::SuperGameBoy;
  }

  if(information.name == "Game Boy" || information.name == "Game Boy Color") {
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
  }
  if(information.name == "Super Game Boy") {
    node = root;
    if(!(pak = node->pak())) return false;
  }
  fastBoot = node->append<Node::Setting::Boolean>("Fast Boot", false);

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  ppu.load(node);
  apu.load(node);
  cartridgeSlot.load(node);
  return true;
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
  cartridgeSlot.unload();
  bootROM.reset();
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  string name = "boot.rom";

  if(GameBoy::Model::GameBoy()) {
    bootROM.allocate(256);
    if(cpu.version->latch() == "DMG-CPU"   ) name = "boot.dmg-0.rom";
    if(cpu.version->latch() == "DMG-CPU A" ) name = "boot.dmg-1.rom";
    if(cpu.version->latch() == "DMG-CPU B" ) name = "boot.dmg-1.rom";
    if(cpu.version->latch() == "DMG-CPU C" ) name = "boot.dmg-1.rom";
    if(cpu.version->latch() == "CPU MGB"   ) name = "boot.mgb.rom";
  }

  if(GameBoy::Model::SuperGameBoy()) {
    bootROM.allocate(256);
    if(cpu.version->latch() == "SGB-CPU 01") name = "sm83.boot.rom";
    if(cpu.version->latch() == "CPU SGB2"  ) name = "sm83.boot.rom";
  }

  if(GameBoy::Model::GameBoyColor()) {
    bootROM.allocate(2048);
    if(cpu.version->latch() == "CPU CGB"   ) name = "boot.cgb-0.rom";
    if(cpu.version->latch() == "CPU CGB A" ) name = "boot.cgb-1.rom";
    if(cpu.version->latch() == "CPU CGB B" ) name = "boot.cgb-1.rom";
    if(cpu.version->latch() == "CPU CGB C" ) name = "boot.cgb-1.rom";
    if(cpu.version->latch() == "CPU CGB D" ) name = "boot.cgb-1.rom";
    if(cpu.version->latch() == "CPU CGB E" ) name = "boot.cgb-1.rom";
  }

  if(auto fp = pak->read(!GameBoy::Model::SuperGameBoy() ? "boot.rom" : "sm83.boot.rom")) {
    bootROM.load(fp);

    if(fastBoot->latch()) {
      if(name == "boot.dmg-1.rom") {
        //skip the first 0x60 loop iterations (~4 seconds) and disable scrolling
        bootROM.program(0x55, 0x26); //ld   h,$60 ; loop upcount   = 0x60
        bootROM.program(0x56, 0x60);
        bootROM.program(0x57, 0x16); //ld   d,$04 ; loop downcount = 0x04
        bootROM.program(0x58, 0x04);
        bootROM.program(0x88, 0x00); //nop        ; disable scroll
      }
      //todo: add fast boot patches for other boot ROMs
    }
  }

  cartridge.power();
  cpu.power();
  ppu.power();
  apu.power();
  scheduler.power(cpu);
}

}
