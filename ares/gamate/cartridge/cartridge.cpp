#include <gamate/gamate.hpp>

namespace ares::Gamate {

Cartridge& cartridge = cartridgeSlot.cartridge;
#include "slot.cpp"
#include "serialization.cpp"

//the key the protection chip expects, held inside the chip itself
static const n8 ProtectionKey[15] = {
  'B', 'I', 'T', ' ', 'C', 'O', 'R', 'P', 'O', 'R', 'A', 'T', 'I', 'O', 'N',
};

auto Cartridge::Protection::power() -> void {
  unlocked = 0;
  failed = 0;
  replying = 0;
  position = 0;
  byte = 0;
  sequence = 0;
}

//bits arrive most significant first. the ninth write of each group is what
//commits the byte just assembled and compares it against the key
auto Cartridge::Protection::write(n1 state) -> void {
  if(position < 8) {
    byte |= state << 7 - position;
    position++;
    return;
  }

  if(!failed && sequence < 15 && byte != ProtectionKey[sequence]) failed = 1;

  position = 0;
  byte = 0;
  sequence++;

  if(!failed && sequence == 15) {
    byte = 0x47;  //the reply the console reads back
    replying = 1;
  }
}

auto Cartridge::Protection::read() -> n1 {
  if(!replying) return 0;

  n1 state = byte >> 7 - position & 1;
  if(++position == 8) {
    unlocked = 1;
    position = 0;
    byte = 0;
  }
  return state;
}

auto Cartridge::allocate(Node::Port parent) -> Node::Peripheral {
  return node = parent->append<Node::Peripheral>(string{system.name(), " Cartridge"});
}

auto Cartridge::connect() -> void {
  if(!node->setPak(pak = platform->pak(node))) return;

  information = {};
  information.title = pak->attribute("title");

  auto board = pak->attribute("board");
  if(board == "Banked") information.board = Board::Banked;
  if(board == "4-in-1") information.board = Board::FourInOne;

  if(auto fp = pak->read("program.rom")) {
    rom.allocate(fp->size());
    rom.load(fp);
  }

  power();
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  pak.reset();
  node.reset();
}

auto Cartridge::save() -> void {
}

auto Cartridge::power() -> void {
  protection.power();
  bank = 0;
  multibank = 0;
  cardAvailable = 0;
}

auto Cartridge::read(n16 offset) -> n8 {
  if(!node) return 0xff;
  if(protection.unlocked) return readROM(offset);
  //before unlocking, the only thing the cartridge drives is one reply bit,
  //and it appears on D1 rather than D0
  return protection.read() << 1;
}

auto Cartridge::write(n16 offset, n8 data) -> void {
  if(!node) return;
  if(protection.unlocked) return writeROM(offset, data);
  return protection.write(data.bit(2));
}

auto Cartridge::readROM(n16 offset) -> n8 {
  if(!rom) return 0xff;

  switch(information.board) {

  case Board::Plain:
    return rom.read(offset % rom.size());

  case Board::Banked:
    //the first 16KB is fixed and the second is switched
    if(offset < 0x4000) return rom.read(offset % rom.size());
    return rom.read((bank * 0x4000 + (offset & 0x3fff)) % rom.size());

  case Board::FourInOne:
    //both halves switch, the lower one selecting which game is running
    if(offset < 0x4000) return rom.read((multibank * 0x4000 + (offset & 0x3fff)) % rom.size());
    return rom.read((bank * 0x4000 + (offset & 0x3fff)) % rom.size());

  }

  return 0xff;
}

auto Cartridge::writeROM(n16 offset, n8 data) -> void {
  if(information.board == Board::FourInOne && offset == 0x2000) {
    multibank = data;
    return;
  }

  if(information.board != Board::Plain && offset == 0x6000) {
    bank = data;
    return;
  }
}

auto Cartridge::cardAvailableSet() -> n8 {
  cardAvailable = 1;
  return 0x00;
}

auto Cartridge::cardAvailableCheck() -> n8 {
  return cardAvailable ? 3 : 1;
}

auto Cartridge::cardReset() -> void {
}

}
