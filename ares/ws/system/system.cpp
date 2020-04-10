#include <ws/ws.hpp>

namespace ares::WonderSwan {

Scheduler scheduler;
System system;
#define Model ares::WonderSwan::Model
#define SoC ares::WonderSwan::SoC
#include "controls.cpp"
#undef Model
#undef SoC
#include "io.cpp"
#include "serialization.cpp"

auto System::run() -> void {
  auto event = scheduler.enter();
  if(event == Event::Frame) ppu.refresh();
  controls.poll();
}

auto System::load(Node::Object& root) -> void {
  if(node) unload();

  information = {};

  if(interface->name() == "WonderSwan"         ) information.soc = SoC::ASWAN,   information.model = Model::WonderSwan;
  if(interface->name() == "WonderSwan Color"   ) information.soc = SoC::SPHINX,  information.model = Model::WonderSwanColor;
  if(interface->name() == "SwanCrystal"        ) information.soc = SoC::SPHINX2, information.model = Model::SwanCrystal;
  if(interface->name() == "Pocket Challenge V2") information.soc = SoC::ASWAN,   information.model = Model::PocketChallengeV2;

  node = Node::System::create(interface->name());
  root = node;

  headphones = node->append<Node::Boolean>("Headphones", true, [&](auto value) {
    apu.r.headphonesConnected = value;
    ppu.updateIcons();
  });
  headphones->setDynamic(true);

  //the EEPROMs come factory-programmed to contain various model names and settings.
  //the model names are confirmed from video recordings of real hardware booting.
  //the other settings bytes are based on how the IPLROMs configure the EEPROMs after changing settings.
  //none of this can be considered 100% verified; direct EEPROM dumps from new-old stock would be required.
  auto initializeName = [&](string name) {
    //16-character limit, 'A'-'Z' only!
    for(uint index : range(name.size())) {
      eeprom.program(0x60 + index, name[index] - 'A' + 0x0b);
    }
  };

  if(WonderSwan::Model::WonderSwan()) {
    if(auto fp = platform->open(node, "boot.rom", File::Read, File::Required)) {
      bootROM.allocate(4_KiB);
      bootROM.load(fp);
    }
    eeprom.allocate(128, 16, 0x00);
    eeprom.program(0x76, 0x01);
    eeprom.program(0x77, 0x00);
    eeprom.program(0x78, 0x24);
    eeprom.program(0x7c, 0x01);
    initializeName("WONDERSWAN");  //verified
  }

  if(WonderSwan::Model::WonderSwanColor()) {
    if(auto fp = platform->open(node, "boot.rom", File::Read, File::Required)) {
      bootROM.allocate(8_KiB);
      bootROM.load(fp);
    }
    eeprom.allocate(2048, 16, 0x00);
    eeprom.program(0x76, 0x01);
    eeprom.program(0x77, 0x01);
    eeprom.program(0x78, 0x27);
    eeprom.program(0x7c, 0x01);
    eeprom.program(0x80, 0x01);
    eeprom.program(0x81, 0x01);
    eeprom.program(0x82, 0x27);
    eeprom.program(0x83, 0x03);  //d0-d1 = volume (0-3); d6 = contrast (0=low, 1=high)
    initializeName("WONDERSWANCOLOR");  //verified
  }

  if(WonderSwan::Model::SwanCrystal()) {
    if(auto fp = platform->open(node, "boot.rom", File::Read, File::Required)) {
      bootROM.allocate(8_KiB);
      bootROM.load(fp);
    }
    eeprom.allocate(2048, 16, 0x00);
    //unverified; based on WonderSwan Color IPLROM
    eeprom.program(0x76, 0x01);
    eeprom.program(0x77, 0x01);
    eeprom.program(0x78, 0x27);
    eeprom.program(0x7c, 0x01);
    eeprom.program(0x80, 0x01);
    eeprom.program(0x81, 0x01);
    eeprom.program(0x82, 0x27);
    eeprom.program(0x83, 0x03);  //d0-d1 = volume (0-3)
    initializeName("SWANCRYSTAL");  //verified

    //Mama Mitte (used by the software to detect the specific Mama Mitte SwanCrystal hardware)
    eeprom.program(0x7a, 0x7f);
    eeprom.program(0x7b, 0x52);
    eeprom.program(0x07fe, 0x34);
    eeprom.program(0x07ff, 0x12);
  }

  if(WonderSwan::Model::PocketChallengeV2()) {
    if(auto fp = platform->open(node, "boot.rom", File::Read, File::Required)) {
      bootROM.allocate(4_KiB);
      bootROM.load(fp);
    }
    //the internal EEPROM has been removed from the Pocket Challenge V2 PCB.
  }

  if(auto fp = platform->open(node, "save.eeprom", File::Read)) {
    fp->read(eeprom.data, eeprom.size);
  }

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  ppu.load(node);
  apu.load(node);
  cartridgeSlot.load(node);
}

auto System::save() -> void {
  if(!node) return;

  if(auto fp = platform->open(node, "save.eeprom", File::Write)) {
    fp->write(eeprom.data, eeprom.size);
  }

  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;

  save();
  bootROM.reset();
  eeprom.reset();
  cpu.unload();
  ppu.unload();
  apu.unload();
  cartridgeSlot.unload();
  node = {};
  headphones = {};
}

auto System::power() -> void {
  for(auto& setting : node->find<Node::Setting>()) setting->setLatch();

  bus.power();
  iram.power();
  eeprom.power();
  cpu.power();
  ppu.power();
  apu.power();
  cartridge.power();
  scheduler.power(cpu);

  information.serializeSize[0] = serializeInit(0);
  information.serializeSize[1] = serializeInit(1);

  bus.map(this, 0x0060);
  bus.map(this, 0x00ba, 0x00be);

  io = {};
}

}
