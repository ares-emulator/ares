#include <gb/gb.hpp>

namespace ares::GameBoy {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "io.cpp"
#include "serialization.cpp"
#include "mbc0/mbc0.cpp"
#include "mbc1/mbc1.cpp"
#include "mbc1m/mbc1m.cpp"
#include "mbc2/mbc2.cpp"
#include "mbc3/mbc3.cpp"
#include "mbc5/mbc5.cpp"
#include "mbc6/mbc6.cpp"
#include "mbc7/mbc7.cpp"
#include "mmm01/mmm01.cpp"
#include "huc1/huc1.cpp"
#include "huc3/huc3.cpp"
#include "tama/tama.cpp"

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(parent->family());
}

auto Cartridge::connect() -> void {
  node->setManifest([&] { return information.manifest; });

  information = {};
  rom = {};
  ram = {};
  rtc = {};
  mapper = &mbc0;
  accelerometer = false;
  rumble = false;

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);
  information.name = document["game/label"].text();

  auto mapperID = document["game/board"].text();
  if(mapperID == "MBC0" ) mapper = &mbc0;
  if(mapperID == "MBC1" ) mapper = &mbc1;
  if(mapperID == "MBC1M") mapper = &mbc1m;
  if(mapperID == "MBC2" ) mapper = &mbc2;
  if(mapperID == "MBC3" ) mapper = &mbc3;
  if(mapperID == "MBC5" ) mapper = &mbc5;
  if(mapperID == "MBC6" ) mapper = &mbc6;
  if(mapperID == "MBC7" ) mapper = &mbc7;
  if(mapperID == "MMM01") mapper = &mmm01;
  if(mapperID == "HuC1" ) mapper = &huc1;
  if(mapperID == "HuC3" ) mapper = &huc3;
  if(mapperID == "TAMA" ) mapper = &tama;

  accelerometer = (bool)document["game/board/accelerometer"];
  rumble = (bool)document["game/board/rumble"];

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(max(0x4000, memory["size"].natural()));
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    ram.allocate(memory["size"].natural());
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Read, File::Optional)) {
        ram.load(fp);
      }
    }
  }

  if(auto memory = document["game/board/memory(type=RTC,content=Time)"]) {
    rtc.allocate(memory["size"].natural());
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "time.rtc", File::Read, File::Optional)) {
        rtc.load(fp);
      }
    }
  }

  mapper->load(node);
  mapper->load(document);

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  mapper->unload();
  rom.reset();
  ram.reset();
  rtc.reset();
  node = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);

  if(auto memory = document["game/board/memory(type=RAM,content=Save)"]) {
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "save.ram", File::Write)) {
        ram.save(fp);
      }
    }
  }

  if(auto memory = document["game/board/memory(type=RTC,content=Time)"]) {
    if(!memory["volatile"]) {
      if(auto fp = platform->open(node, "time.rtc", File::Write)) {
        rtc.save(fp);
      }
    }
  }

  mapper->save(document);
}

auto Cartridge::power() -> void {
  Thread::create(4 * 1024 * 1024, {&Cartridge::main, this});

  bootromEnable = true;

  if(mapper) mapper->power();
}

auto Cartridge::main() -> void {
  mapper->main();
}

auto Cartridge::step(uint clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

auto Cartridge::second() -> void {
  mapper->second();
}

auto Cartridge::Mapper::main() -> void {
  cartridge.step(cartridge.frequency());
}

}
