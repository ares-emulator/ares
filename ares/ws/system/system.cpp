#include <ws/ws.hpp>

namespace ares::WonderSwan {

auto enumerate() -> vector<string> {
  return {
    "[Bandai] WonderSwan",
    "[Bandai] WonderSwan Color",
    "[Bandai] SwanCrystal",
    "[Benesse] Pocket Challenge V2",
  };
}

auto load(Node::System& node, string name) -> bool {
  if(!enumerate().find(name)) return false;
  return system.load(node, name);
}

Scheduler scheduler;
System system;
#define Model ares::WonderSwan::Model
#define SoC ares::WonderSwan::SoC
#include "controls.cpp"
#undef Model
#undef SoC
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto System::game() -> string {
  if(cartridge.node) {
    return cartridge.title();
  }

  return "(no cartridge connected)";
}

auto System::run() -> void {
  scheduler.enter();
  controls.poll();
}

auto System::load(Node::System& root, string name) -> bool {
  if(node) unload();

  information = {};
  if(name.find("WonderSwan")) {
    information.name = "WonderSwan";
    information.soc = SoC::ASWAN;
    information.model = Model::WonderSwan;
  }
  if(name.find("WonderSwan Color")) {
    information.name = "WonderSwan Color";
    information.soc = SoC::SPHINX;
    information.model = Model::WonderSwanColor;
  }
  if(name.find("SwanCrystal")) {
    information.name = "SwanCrystal";
    information.soc = SoC::SPHINX2;
    information.model = Model::SwanCrystal;
  }
  if(name.find("Pocket Challenge V2")) {
    information.name = "Pocket Challenge V2";
    information.soc = SoC::ASWAN;
    information.model = Model::PocketChallengeV2;
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

  headphones = node->append<Node::Setting::Boolean>("Headphones", true, [&](auto value) {
    apu.io.headphonesConnected = value;
    ppu.updateIcons();
  });
  headphones->setDynamic(true);

  //the EEPROMs come factory-programmed to contain various model names and settings.
  //the model names are confirmed from video recordings of real hardware booting.
  //the other settings bytes are based on how the IPLROMs configure the EEPROMs after changing settings.
  //none of this can be considered 100% verified; direct EEPROM dumps from new-old stock would be required.
  auto initializeName = [&](string name) {
    //16-character limit, 'A'-'Z' only!
    for(u32 index : range(name.size())) {
      eeprom.program(0x60 + index, name[index] - 'A' + 0x0b);
    }
  };

  if(WonderSwan::Model::WonderSwan()) {
    if(auto fp = pak->read("boot.rom")) {
      bootROM.allocate(4_KiB);
      bootROM.load(fp);
    }
    eeprom.allocate(128, 16, 1, 0x00);
    eeprom.program(0x76, 0x01);
    eeprom.program(0x77, 0x00);
    eeprom.program(0x78, 0x24);
    eeprom.program(0x7c, 0x01);
    initializeName("WONDERSWAN");  //verified
  }

  if(WonderSwan::Model::WonderSwanColor()) {
    if(auto fp = pak->read("boot.rom")) {
      bootROM.allocate(8_KiB);
      bootROM.load(fp);
    }
    eeprom.allocate(2048, 16, 1, 0x00);
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
    if(auto fp = pak->read("boot.rom")) {
      bootROM.allocate(8_KiB);
      bootROM.load(fp);
    }
    eeprom.allocate(2048, 16, 1, 0x00);
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
    if(auto fp = pak->read("boot.rom")) {
      bootROM.allocate(4_KiB);
      bootROM.load(fp);
    }
    //the internal EEPROM has been removed from the Pocket Challenge V2 PCB.
  }

  if(auto fp = pak->read("save.eeprom")) {
    if(fp->attribute("loaded").boolean()) {
      fp->read({eeprom.data, eeprom.size});
    }
  }

  scheduler.reset();
  controls.load(node);
  cpu.load(node);
  ppu.load(node);
  apu.load(node);
  cartridgeSlot.load(node);
  debugger.load(node);
  return true;
}

auto System::save() -> void {
  if(!node) return;

  if(auto fp = pak->write("save.eeprom")) {
    fp->write({eeprom.data, eeprom.size});
  }

  cartridge.save();
}

auto System::unload() -> void {
  if(!node) return;

  save();
  debugger.unload(node);
  bootROM.reset();
  eeprom.reset();
  cpu.unload();
  ppu.unload();
  apu.unload();
  cartridgeSlot.unload();
  headphones.reset();
  pak.reset();
  node.reset();
}

auto System::power(bool reset) -> void {
  for(auto& setting : node->find<Node::Setting::Setting>()) setting->setLatch();

  bus.power();
  iram.power();
  eeprom.power();
  cpu.power();
  ppu.power();
  apu.power();
  cartridge.power();
  scheduler.power(cpu);

  bus.map(this, 0x0060);
  bus.map(this, 0x00ba, 0x00be);

  io = {};
}

}
