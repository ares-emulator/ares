#include "eeprom.cpp"
#include "serialization.cpp"

auto Cartridge::MBC7::load(Node::Object parent) -> void {
  x = parent->append<Node::Axis>("X");
  y = parent->append<Node::Axis>("Y");
}

auto Cartridge::MBC7::load(Markup::Node document) -> void {
  eeprom.load(document);
}

auto Cartridge::MBC7::save(Markup::Node document) -> void {
  eeprom.save(document);
}

auto Cartridge::MBC7::main() -> void {
  eeprom.main();
}

auto Cartridge::MBC7::read(uint16 address) -> uint8 {
  if((address & 0xc000) == 0x0000) {  //$0000-3fff
    return cartridge.rom.read((uint14)address);
  }

  if((address & 0xc000) == 0x4000) {  //$4000-7fff
    return cartridge.rom.read(io.rom.bank << 14 | (uint14)address);
  }

  if((address & 0xf000) == 0xa000) {  //$a000-afff
    if(!io.ram.enable[0] || !io.ram.enable[1]) return 0xff;

    switch(address.bit(4,7)) {
    case 2: return io.accelerometer.x.byte(0);
    case 3: return io.accelerometer.x.byte(1);
    case 4: return io.accelerometer.y.byte(0);
    case 5: return io.accelerometer.y.byte(1);
    case 6: return 0x00;  //z?
    case 7: return 0xff;  //z?
    case 8: return eeprom.readIO();
    }

    return 0xff;
  }

  return 0xff;
}

auto Cartridge::MBC7::write(uint16 address, uint8 data) -> void {
  if((address & 0xe000) == 0x0000) {  //$0000-1fff
    io.ram.enable[0] = data.bit(0,3) == 0xa;
    if(!io.ram.enable[0]) io.ram.enable[1] = false;
    return;
  }

  if((address & 0xe000) == 0x2000) {  //$2000-3fff
    io.rom.bank = data;
    if(!io.rom.bank) io.rom.bank = 1;
    return;
  }

  if((address & 0xe000) == 0x4000) {  //$4000-5fff
    if(!io.ram.enable[0]) return;
    io.ram.enable[1] = data == 0x40;
  }

  if((address & 0xf000) == 0xa000) {  //$a000-afff
    if(!io.ram.enable[0] || !io.ram.enable[1]) return;

    switch(address.bit(4,7)) {
    case 0: {
      if(data != 0x55) break;
      io.accelerometer.x = Center;
      io.accelerometer.y = Center;
      break;
    }

    case 1: {
      if(data != 0xaa) break;
      platform->input(x);
      platform->input(y);
      io.accelerometer.x = Center - x->value();
      io.accelerometer.y = Center + y->value();
      break;
    }

    case 8: {
      eeprom.writeIO(data);
      break;
    }
    }

    return;
  }
}

auto Cartridge::MBC7::power() -> void {
  eeprom.power();
  io = {};
}
