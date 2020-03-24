auto Flash::reset(natural ID) -> void {
  this->ID = ID;
  rom.reset();
  modified = false;
  vendorID = 0;
  deviceID = 0;
  blocks.reset();
}

auto Flash::allocate(natural size) -> bool {
  if(size ==  4_Mibit) { rom.allocate(size); vendorID = 0x98; deviceID = 0xab; }  //vendorID 0x98 => Toshiba
  if(size ==  8_Mibit) { rom.allocate(size); vendorID = 0x98; deviceID = 0x2c; }  //vendorID 0xb0 => Sharp
  if(size == 16_Mibit) { rom.allocate(size); vendorID = 0x98; deviceID = 0x2f; }  //vendorID 0xec => Samsung
  if(!rom) return false;

  for(uint index : range(size / 64_KiB - 1)) blocks.append({true, index * 64_KiB, 64_KiB});
  blocks.append({true, size - 64_KiB, 32_KiB});
  blocks.append({true, size - 32_KiB,  8_KiB});
  blocks.append({true, size - 24_KiB,  8_KiB});
  blocks.append({true, size - 16_KiB, 16_KiB});
  return true;
}

auto Flash::load(shared_pointer<vfs::file> fp) -> void {
  fp->read(rom.data(), rom.size());
  modified = false;
}

auto Flash::save(shared_pointer<vfs::file> fp) -> void {
  fp->write(rom.data(), rom.size());
  modified = false;
}

auto Flash::power() -> void {
  status(Read);
}

auto Flash::read(uint21 address) -> uint8 {
  if(mode == ReadID) {
    switch((uint14)address) {  //todo: actual mask value unknown
    case 0: return vendorID;
    case 1: return deviceID;
    case 2: return 0x02;  //unknown purpose
    case 3: return 0x80;  //unknown purpose
    }
    return 0xff;  //invalid ReadID address; todo: actual return value unknown
  }
  return rom.read(address);  //todo: what happens when mode != Read here?
}

auto Flash::write(uint21 address, uint8 data) -> void {
  if(mode == Write) return program(address, data);
  if(data == 0xf0) return status(Read);
  uint15 addr = (uint15)address;
  if(index == 0 && addr == 0x5555 && data == 0xaa) return status(Index);
  if(index == 1 && addr == 0x2aaa && data == 0x55) return status(Index);
  //todo: erase and protect diverge here; but they're treated the same for simplicity for now
  if(index == 2 && addr == 0x5555 && data == 0x80) return status(Index);
  if(index == 2 && addr == 0x5555 && data == 0x9a) return status(Index);
  if(index == 2 && addr == 0x5555 && data == 0x90) return status(ReadID);
  if(index == 2 && addr == 0x5555 && data == 0xa0) return status(Write);
  if(index == 2 && addr == 0x5555 && data == 0xf0) return status(Read);
  if(index == 3 && addr == 0x5555 && data == 0xaa) return status(Index);
  if(index == 4 && addr == 0x2aaa && data == 0x55) return status(Index);
  if(index == 5 && addr == 0x5555 && data == 0x10) return eraseAll();
  if(index == 5                   && data == 0x30) return erase(address);
  if(index == 5                   && data == 0x9a) return protect(address);
  return status(Read);
}

auto Flash::status(uint mode_) -> void {
  mode = mode_;
  if(mode == Read) index = 0;
  if(mode == Index) index++;
}

auto Flash::block(uint21 address) -> maybe<uint6> {
  for(uint6 index : range(blocks.size())) {
    if(address <  blocks[index].offset) continue;
    if(address >= blocks[index].offset + blocks[index].length) continue;
    return index;
  }
  //todo: it is unknown what happens when protecting and erasing invalid block IDs
  return {};
}

auto Flash::program(uint21 address, uint8 data) -> void {
  auto blockID = block(address);
  if(blockID && blocks[*blockID].writable) {
    if(auto input = rom.read(address); input != (input & data)) {
      modified = true;
      rom.write(address, input & data);
    }
  }
  return status(Read);
}

auto Flash::protect(uint21 address) -> void {
  auto blockID = block(address);
  if(blockID && blocks[*blockID].writable) {
    blocks[*blockID].writable = false;
    modified = true;
  }
  return status(Read);
}

auto Flash::erase(uint21 address) -> void {
  auto blockID = block(address);
  if(blockID && blocks[*blockID].writable) {
    auto address = blocks[*blockID].offset;
    auto length = blocks[*blockID].length;
    for(uint offset : range(length)) rom.write(address + offset, 0xff);
    modified = true;
  }
  return status(Read);
}

auto Flash::eraseAll() -> void {
  for(auto& block : blocks) erase(block.offset);
}
