auto VPU::read(uint24 address) -> uint8 {
  address = 0x8000 | (uint14)address;
  if(address >= 0x8200 && address <= 0x83ff && Model::NeoGeoPocketColor()) return readColor(address);
  if(address >= 0x8800 && address <= 0x88ff) return readSprite(address);
  if(address >= 0x8c00 && address <= 0x8c3f && Model::NeoGeoPocketColor()) return readSpriteColor(address);
  if(address >= 0x9000 && address <= 0x9fff) return readAttribute(address);
  if(address >= 0xa000 && address <= 0xbfff) return readCharacter(address);

  uint8 data = 0x00;

  switch(address) {
  case 0x8000:
    data.bit(6) = io.hblankEnableIRQ;
    data.bit(7) = io.vblankEnableIRQ;
    break;

  case 0x8002: data = window.hoffset; break;
  case 0x8003: data = window.voffset; break;
  case 0x8004: data = window.hlength; break;
  case 0x8005: data = window.vlength; break;

  case 0x8006: data = io.vlines; break;

  case 0x8008: data = io.hcounter.bit(2,9); break;
  case 0x8009: data = io.vcounter.bit(0,7); break;

  case 0x8010:
    data.bit(6) = io.vblankActive;
    data.bit(7) = io.characterOver;
    break;

  case 0x8012:
    data.bit(0,2) = window.color;
    data.bit(7) = dac.negate;
    break;

  case 0x8020: data = sprite.hscroll; break;
  case 0x8021: data = sprite.vscroll; break;

  case 0x8030: data.bit(7) = io.planePriority; break;

  case 0x8032: data = plane1.hscroll; break;
  case 0x8033: data = plane1.vscroll; break;
  case 0x8034: data = plane2.hscroll; break;
  case 0x8035: data = plane2.vscroll; break;

  case 0x8100: data.bit(0,2) = sprite.palette[0][0]; break;
  case 0x8101: data.bit(0,2) = sprite.palette[0][1]; break;
  case 0x8102: data.bit(0,2) = sprite.palette[0][2]; break;
  case 0x8103: data.bit(0,2) = sprite.palette[0][3]; break;
  case 0x8104: data.bit(0,2) = sprite.palette[1][0]; break;
  case 0x8105: data.bit(0,2) = sprite.palette[1][1]; break;
  case 0x8106: data.bit(0,2) = sprite.palette[1][2]; break;
  case 0x8107: data.bit(0,2) = sprite.palette[1][3]; break;

  case 0x8108: data.bit(0,2) = plane1.palette[0][0]; break;
  case 0x8109: data.bit(0,2) = plane1.palette[0][1]; break;
  case 0x810a: data.bit(0,2) = plane1.palette[0][2]; break;
  case 0x810b: data.bit(0,2) = plane1.palette[0][3]; break;
  case 0x810c: data.bit(0,2) = plane1.palette[1][0]; break;
  case 0x810d: data.bit(0,2) = plane1.palette[1][1]; break;
  case 0x810e: data.bit(0,2) = plane1.palette[1][2]; break;
  case 0x810f: data.bit(0,2) = plane1.palette[1][3]; break;

  case 0x8110: data.bit(0,2) = plane2.palette[0][0]; break;
  case 0x8111: data.bit(0,2) = plane2.palette[0][1]; break;
  case 0x8112: data.bit(0,2) = plane2.palette[0][2]; break;
  case 0x8113: data.bit(0,2) = plane2.palette[0][3]; break;
  case 0x8114: data.bit(0,2) = plane2.palette[1][0]; break;
  case 0x8115: data.bit(0,2) = plane2.palette[1][1]; break;
  case 0x8116: data.bit(0,2) = plane2.palette[1][2]; break;
  case 0x8117: data.bit(0,2) = plane2.palette[1][3]; break;

  case 0x8118:
    if(!Model::NeoGeoPocketColor()) break;
    data.bit(0,2) = background.color;
    data.bit(3,5) = background.unused;
    data.bit(6,7) = background.mode;
    break;

  case 0x8400: data = led.control; break;
  case 0x8402: data = led.frequency; break;

  case 0x87e2:
    if(!Model::NeoGeoPocketColor()) break;
    data.bit(0,6) = 0b000000;
    data.bit(7) = dac.colorMode;
    break;

  case 0x87fe: data = 0x3f; break;  //input port register (reserved)
  }

  return data;
}

auto VPU::write(uint24 address, uint8 data) -> void {
  address = 0x8000 | (uint14)address;
  if(address >= 0x8200 && address <= 0x83ff && Model::NeoGeoPocketColor()) return writeColor(address, data);
  if(address >= 0x8800 && address <= 0x88ff) return writeSprite(address, data);
  if(address >= 0x8c00 && address <= 0x8cff && Model::NeoGeoPocketColor()) return writeSpriteColor(address, data);
  if(address >= 0x9000 && address <= 0x9fff) return writeAttribute(address, data);
  if(address >= 0xa000 && address <= 0xbfff) return writeCharacter(address, data);

  switch(address) {
  case 0x8000:
    io.hblankEnableIRQ = data.bit(6);
    io.vblankEnableIRQ = data.bit(7);
    break;

  case 0x8002: window.hoffset = data; break;
  case 0x8003: window.voffset = data; break;
  case 0x8004: window.hlength = data; break;
  case 0x8005: window.vlength = data; break;

  case 0x8006:
    io.vlines = max(152, data);  //it is unknown if this value can be set below the height of the LCD screen
    break;

  case 0x8012:
    window.color = data.bit(0,2);
    dac.negate = data.bit(7);
    break;

  case 0x8020: sprite.hscroll = data; break;
  case 0x8021: sprite.vscroll = data; break;

  case 0x8030: io.planePriority = data.bit(7); break;

  case 0x8032: plane1.hscroll = data; break;
  case 0x8033: plane1.vscroll = data; break;
  case 0x8034: plane2.hscroll = data; break;
  case 0x8035: plane2.vscroll = data; break;

  case 0x8100: break;
  case 0x8101: sprite.palette[0][1] = data.bit(0,2); break;
  case 0x8102: sprite.palette[0][2] = data.bit(0,2); break;
  case 0x8103: sprite.palette[0][3] = data.bit(0,2); break;
  case 0x8104: break;
  case 0x8105: sprite.palette[1][1] = data.bit(0,2); break;
  case 0x8106: sprite.palette[1][2] = data.bit(0,2); break;
  case 0x8107: sprite.palette[1][3] = data.bit(0,2); break;

  case 0x8108: break;
  case 0x8109: plane1.palette[0][1] = data.bit(0,2); break;
  case 0x810a: plane1.palette[0][2] = data.bit(0,2); break;
  case 0x810b: plane1.palette[0][3] = data.bit(0,2); break;
  case 0x810c: break;
  case 0x810d: plane1.palette[1][1] = data.bit(0,2); break;
  case 0x810e: plane1.palette[1][2] = data.bit(0,2); break;
  case 0x810f: plane1.palette[1][3] = data.bit(0,2); break;

  case 0x8110: break;
  case 0x8111: plane2.palette[0][1] = data.bit(0,2); break;
  case 0x8112: plane2.palette[0][2] = data.bit(0,2); break;
  case 0x8113: plane2.palette[0][3] = data.bit(0,2); break;
  case 0x8114: break;
  case 0x8115: plane2.palette[1][1] = data.bit(0,2); break;
  case 0x8116: plane2.palette[1][2] = data.bit(0,2); break;
  case 0x8117: plane2.palette[1][3] = data.bit(0,2); break;

  case 0x8118:
    if(!Model::NeoGeoPocketColor()) break;
    background.color  = data.bit(0,2);
    background.unused = data.bit(3,5);
    background.mode   = data.bit(6,7);
    break;

  case 0x8400: led.control.bit(3,7) = data.bit(3,7); break;
  case 0x8402: led.frequency = data; break;

  case 0x87e0:
    if(data == 0x52) io = {};
    break;

  case 0x87e2:
    if(!Model::NeoGeoPocketColor()) break;
    if(!cpu.privilegedMode()) break;  //user-mode code is not supposed to be able to write to this register
    dac.colorMode = data.bit(7);
    break;
  }
}

auto VPU::readSprite(uint8 address) -> uint8 {
  auto& s = sprites[address >> 2];
  switch(address & 3) {
  case 0: return s.character;
  case 1: return s.character >> 8 | s.vchain << 1 | s.hchain << 2 | s.priority << 3 | s.palette << 5 | s.vflip << 6 | s.hflip << 7;
  case 2: return s.hoffset;
  case 3: return s.voffset;
  }
  unreachable;
}

auto VPU::writeSprite(uint8 address, uint8 data) -> void {
  auto& s = sprites[address >> 2];
  switch(address & 3) {
  case 0:
    s.character.bit(0,7) = data;
    break;
  case 1:
    s.character.bit(8) = data.bit(0);
    s.vchain = data.bit(1);
    s.hchain = data.bit(2);
    s.priority = data.bit(3,4);
    s.palette = data.bit(5);
    s.vflip = data.bit(6);
    s.hflip = data.bit(7);
    break;
  case 2:
    s.hoffset = data;
    break;
  case 3:
    s.voffset = data;
    break;
  }
}

auto VPU::readSpriteColor(uint6 address) -> uint8 {
  return sprites[address].code;  //d4-d7 = 0
}

auto VPU::writeSpriteColor(uint6 address, uint8 data) -> void {
  sprites[address].code = data.bit(0,3);
}

auto VPU::readColor(uint9 address) -> uint8 {
  auto& p = colors[address >> 1];
  if(!address.bit(0)) {
    return p.bit(0, 7);
  } else {
    return p.bit(8,11);  //d4-d7 = 0
  }
}

auto VPU::writeColor(uint9 address, uint8 data) -> void {
  auto& p = colors[address >> 1];
  if(!address.bit(0)) {
    p.bit(0, 7) = data.bit(0,7);
  } else {
    p.bit(8,11) = data.bit(0,3);
  }
}

auto VPU::readAttribute(uint12 address) -> uint8 {
  auto& a = attributes[address >> 1];
  if(!address.bit(0)) {
    return a.character.bit(0,7);
  } else {
    return a.character.bit(8) << 0 | a.code << 1 | a.palette << 5 | a.vflip << 6 | a.hflip << 7;
  }
}

auto VPU::writeAttribute(uint12 address, uint8 data) -> void {
  auto& a = attributes[address >> 1];
  if(!address.bit(0)) {
    a.character.bit(0,7) = data.bit(0,7);
  } else {
    a.character.bit(8) = data.bit(0);
    a.code = data.bit(1,4);
    a.palette = data.bit(5);
    a.vflip = data.bit(6);
    a.hflip = data.bit(7);
  }
}

auto VPU::readCharacter(uint13 address) -> uint8 {
  auto& c = characters[address >> 4];
  uint3 y = address >> 1;
  uint3 x = address >> 0 << 2;
  uint8 data;
  data.bit(0,1) = c[y][x++];
  data.bit(2,3) = c[y][x++];
  data.bit(4,5) = c[y][x++];
  data.bit(6,7) = c[y][x++];
  return data;
}

auto VPU::writeCharacter(uint13 address, uint8 data) -> void {
  auto& c = characters[address >> 4];
  uint3 y = address >> 1;
  uint3 x = address >> 0 << 2;
  c[y][x++] = data.bit(0,1);
  c[y][x++] = data.bit(2,3);
  c[y][x++] = data.bit(4,5);
  c[y][x++] = data.bit(6,7);
}
