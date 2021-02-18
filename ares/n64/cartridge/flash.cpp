auto Cartridge::Flash::readHalf(u32 address) -> u16 {
  u16 data = 0;

  if(mode == Mode::Status) {
    data = status >> ((address >> 1 & 3) ^ 3) * 16;
  }

  if(mode == Mode::Read) {
    data = Memory::Writable::readHalf(address);
  }

  return data;
}

auto Cartridge::Flash::readWord(u32 address) -> u32 {
  return status >> 32;
}

auto Cartridge::Flash::writeHalf(u32 address, u16 data) -> void {
  if(mode == Mode::Write) {
    //writes are deferred until the flash execute command is sent later
    source = pi.io.dramAddress;
  }
}

auto Cartridge::Flash::writeWord(u32 address, u32 data) -> void {
  u8 command = data >> 24;

  //set erase offset
  if(command == 0x4b) {
    offset = u16(data) * 128;
    return;
  }

  //erase
  if(command == 0x78) {
    mode = Mode::Erase;
    status = 0x1111'8008'00c2'0000ull;
    return;
  }

  //set write offset
  if(command == 0xa5) {
    offset = u16(data) * 128;
    status = 0x1111'8004'00c2'0000ull;
    return;
  }

  //write
  if(command == 0xb4) {
    mode = Mode::Write;
    return;
  }

  //execute
  if(command == 0xd2) {
    if(mode == Mode::Erase) {
      for(u32 index = 0; index < 128; index += 2) {
        Memory::Writable::writeHalf(offset + index, 0xffff);
      }
    }
    if(mode == Mode::Write) {
      for(u32 index = 0; index < 128; index += 2) {
        u16 half = rdram.ram.readHalf(source + index);
        Memory::Writable::writeHalf(offset + index, half);
      }
    }
    return;
  }

  //status
  if(command == 0xe1) {
    mode = Mode::Status;
    status = 0x1111'8001'00c2'0000ull;
    return;
  }

  //read
  if(command == 0xf0) {
    mode = Mode::Read;
    status = 0x1111'8004'f000'0000ull;
    return;
  }
}
