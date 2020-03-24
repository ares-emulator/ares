#include <md/md.hpp>

namespace ares::MegaDrive {

Cartridge cartridge;
#include "serialization.cpp"

auto Cartridge::load(Node::Object parent, Node::Object from) -> void {
  port = Node::append<Node::Port>(parent, from, "Cartridge Slot");
  port->setFamily("Mega Drive");
  port->setType("Cartridge");
  port->setAllocate([&] { return Node::Peripheral::create(interface->name()); });
  port->setAttach([&](auto node) { connect(node); });
  port->setDetach([&](auto node) { disconnect(); });
  port->scan(from);
}

auto Cartridge::unload() -> void {
  disconnect();
  port = {};
}

auto Cartridge::connect(Node::Peripheral with) -> void {
  node = Node::append<Node::Peripheral>(port, with, interface->name());
  node->setManifest([&] { return information.manifest; });

  information = {};

  if(auto fp = platform->open(node, "manifest.bml", File::Read, File::Required)) {
    information.manifest = fp->reads();
  }

  auto document = BML::unserialize(information.manifest);

  if(!loadROM(rom, document["game/board/memory(type=ROM,content=Program)"])) rom.reset();
  if(!loadROM(patch, document["game/board/memory(type=ROM,content=Patch)"])) patch.reset();
  if(!loadRAM(wram, document["game/board/memory(type=RAM,content=Save)"])) wram.reset();
  if(!loadRAM(bram, document["game/board/memory(type=RAM,content=Save)"])) bram.reset();

  information.name = document["game/label"].text();
  information.regions = document["game/region"].text().split(",").strip();
  information.bootable = (bool)document["game/bootable"];

  read = {&Cartridge::readLinear, this};
  write = {&Cartridge::writeLinear, this};

  if(rom.size() > 0x200000) {
    read = {&Cartridge::readBanked, this};
    write = {&Cartridge::writeBanked, this};
  }

  if(document["game/board/slot(type=Mega Drive)"]) {
    slot = new Cartridge{depth + 1};
    slot->load(node, with);

    if(patch) {
      read = {&Cartridge::readLockOn, this};
      write = {&Cartridge::writeLockOn, this};
    } else {
      read = {&Cartridge::readGameGenie, this};
      write = {&Cartridge::writeGameGenie, this};
    }
  }

  //easter egg: power draw increases with each successively stacked cartridge
  //simulate increasing address/data line errors as stacking increases
  if(depth >= 3) {
    auto reader = read;
    auto writer = write;
    auto scramble = [=](uint32 value) -> uint32 {
      uint chance = max(1, (1 << 19) >> depth) - 1;
      if((random() & chance) == 0) value ^= 1 << (random() & 31);
      return value;
    };
    read = [=](uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
      return scramble(reader(upper, lower, scramble(address), data));
    };
    write = [=](uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
      writer(upper, lower, scramble(address), scramble(data));
    };
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  patch.reset();
  wram.reset();
  bram.reset();
  read.reset();
  write.reset();
  if(slot) slot->disconnect();
  slot.reset();
  node = {};
  information = {};
}

auto Cartridge::save() -> void {
  if(!node) return;
  auto document = BML::unserialize(information.manifest);
  saveRAM(wram, document["game/board/memory(type=RAM,content=Save)"]);
  saveRAM(bram, document["game/board/memory(type=RAM,content=Save)"]);
  if(slot) slot->save();
}

auto Cartridge::power() -> void {
  ramEnable = 1;
  ramWritable = 1;
  for(uint n : range(8)) romBank[n] = n;
  gameGenie = {};
}

//

auto Cartridge::loadROM(Memory::Readable<uint16>& rom, Markup::Node memory) -> bool {
  if(!memory) return false;

  auto name = string{memory["content"].text(), ".", memory["type"].text()}.downcase();
  rom.allocate(memory["size"].natural() >> 1);
  if(auto fp = platform->open(node, name, File::Read, File::Required)) {
    for(uint address : range(rom.size())) rom.program(address, fp->readm(2));
  }

  return true;
}

auto Cartridge::loadRAM(Memory::Writable<uint16>& ram, Markup::Node memory) -> bool {
  if(!memory) return false;
  if(memory["mode"].text() != "word") return false;

  ramUpper = 1;
  ramLower = 1;
  auto name = string{memory["content"].text(), ".", memory["type"].text()}.downcase();
  ram.allocate(memory["size"].natural() >> 1);
  if(!memory["volatile"]) {
    if(auto fp = platform->open(node, name, File::Read)) {
      for(uint address : range(ram.size())) ram.write(address, fp->readm(2));
    }
  }

  return true;
}

auto Cartridge::loadRAM(Memory::Writable<uint8>& ram, Markup::Node memory) -> bool {
  if(!memory) return false;
  if(memory["mode"].text() == "word") return false;

  ramUpper = memory["mode"].text() == "hi";
  ramLower = memory["mode"].text() == "lo";
  auto name = string{memory["content"].text(), ".", memory["type"].text()}.downcase();
  ram.allocate(memory["size"].natural());
  if(!memory["volatile"]) {
    if(auto fp = platform->open(node, name, File::Read)) {
      for(uint address : range(ram.size())) ram.write(address, fp->readm(1));
    }
  }

  return true;
}

auto Cartridge::saveRAM(Memory::Writable<uint16>& ram, Markup::Node memory) -> bool {
  if(!memory) return false;
  if(memory["volatile"]) return true;

  auto name = string{memory["content"].text(), ".", memory["type"].text()}.downcase();
  if(auto fp = platform->open(node, name, File::Write)) {
    for(uint address : range(ram.size())) fp->writem(ram[address], 2);
  } else return false;

  return true;
}

auto Cartridge::saveRAM(Memory::Writable<uint8>& ram, Markup::Node memory) -> bool {
  if(!memory) return false;
  if(memory["volatile"]) return true;

  auto name = string{memory["content"].text(), ".", memory["type"].text()}.downcase();
  if(auto fp = platform->open(node, name, File::Write)) {
    for(uint address : range(ram.size())) fp->writem(ram[address], 1);
  } else return false;

  return true;
}

//

auto Cartridge::readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16 {
  if(slot) slot->readIO(upper, lower, address, data);
  return data;
}

auto Cartridge::writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void {
  if(slot) slot->writeIO(upper, lower, address, data);
  if(!lower) return;  //todo: unconfirmed
  if(address == 0xa130f0) ramEnable = data.bit(0), ramWritable = data.bit(1);
  if(address == 0xa130f2) romBank[1] = data;
  if(address == 0xa130f4) romBank[2] = data;
  if(address == 0xa130f6) romBank[3] = data;
  if(address == 0xa130f8) romBank[4] = data;
  if(address == 0xa130fa) romBank[5] = data;
  if(address == 0xa130fc) romBank[6] = data;
  if(address == 0xa130fe) romBank[7] = data;
}

//

auto Cartridge::readLinear(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
  if(address >= 0x200000 && ramEnable) {
    if(wram) return data = wram[address >> 1];
    if(bram) return data = bram[address >> 1] * 0x0101;  //todo: unconfirmed
  }
  return data = rom[address >> 1];
}

auto Cartridge::writeLinear(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
  //emulating ramWritable will break commercial software:
  //it does not appear that many (any?) games actually connect $a130f1.d1 to /WE;
  //hence RAM ends up always being writable, and many games fail to set d1=1
  if(address >= 0x200000 && ramEnable) {
    if(wram) {
      if(upper) wram[address >> 1].byte(1) = data.byte(1);
      if(lower) wram[address >> 1].byte(0) = data.byte(0);
      return;
    }
    if(bram) {
      bram[address >> 1] = data;  //todo: unconfirmed
      return;
    }
  }
}

//

auto Cartridge::readBanked(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
  uint25 offset = romBank[address.bit(19,21)] << 19 | address.bit(0,18);
  return data = rom[offset >> 1];
}

auto Cartridge::writeBanked(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
}

//

auto Cartridge::readLockOn(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
  if(address < 0x200000) return data = rom[address >> 1];
  if(address >= 0x300000 && ramEnable) return data = patch[address >> 1];
  if(slot && slot->node) return data = slot->read(upper, lower, address, data);
  return data;
}

auto Cartridge::writeLockOn(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
  if(slot && slot->node) return slot->write(upper, lower, address, data);
}

//

auto Cartridge::readGameGenie(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16 {
  if(gameGenie.enable) {
    for(auto& code : gameGenie.codes) {
      if(code.enable && code.address == address) return data = code.data;
    }
    if(slot && slot->node) return slot->read(upper, lower, address, data);
  }
  return data = rom[address >> 1];
}

auto Cartridge::writeGameGenie(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void {
  if(gameGenie.enable) {
    if(slot && slot->node) return slot->write(upper, lower, address, data);
  }
  if(address == 0x02 && data == 0x0001) {
    gameGenie.enable = true;
  }
  if(address >= 0x04 && address <= 0x20 && upper && lower) {  //todo: what about 8-bit writes?
    address = address - 0x04 >> 1;
    auto& code = gameGenie.codes[address / 3];
    if(address % 3 == 0) code.address.bit(16,23) = data.bit(0, 7);
    if(address % 3 == 1) code.address.bit( 0,15) = data.bit(0,15);
    if(address % 3 == 2) code.data = data, code.enable = true;
  }
}

}
