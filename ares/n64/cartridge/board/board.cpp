namespace Board {

#include "nus-01a.cpp"
#include "nus-07a.cpp"
#include "generic.cpp"

auto Interface::title() const -> string {
  return cartridge.information.title;
}

auto Interface::cic() const -> string {
  return cartridge.information.cic;
}

template<typename T>
auto Interface::load(T& rom, string name) -> bool {
  rom.reset();
  if(auto fp = pak->read(name)) {
    rom.allocate(fp->size());
    rom.load(fp);
    return true;
  }
  return false;
}

template<typename T>
auto Interface::save(T& rom, string name) -> bool {  
  if(auto fp = pak->write(name)) {
    rom.save(fp);
    return true;
  }
  return false;
}

auto Interface::joybusEeprom(Memory::Writable16& eeprom, n8 send, n8 recv, n8 input[], n8 output[]) -> n1 {
  n1 valid = 0;

  //status
  if(input[0] == 0x00 || input[0] == 0xff) {
    //cartridge EEPROM (4kbit)
    if(eeprom.size == 512) {
      output[0] = 0x00;
      output[1] = 0x80;
      output[2] = 0x00;
      valid = 1;
    }

    //cartridge EEPROM (16kbit)
    if(eeprom.size == 2048) {
      output[0] = 0x00;
      output[1] = 0xc0;
      output[2] = 0x00;
      valid = 1;
    }
  }

  //read EEPROM
  if(input[0] == 0x04 && send >= 2) {
    u32 address = input[1] * 8;
    for(u32 index : range(recv)) {
      output[index] = eeprom.read<Byte>(address++);
    }
    valid = 1;
  }

  //write EEPROM
  if(input[0] == 0x05 && send >= 2 && recv >= 1) {
    u32 address = input[1] * 8;
    for(u32 index : range(send - 2)) {
      eeprom.write<Byte>(address++, input[2 + index]);
    }
    output[0] = 0x00;
    valid = 1;
  }

  return valid;
}

}