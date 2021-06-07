auto KGE::read(n24 address) -> n8 {
  address = 0x8000 | (n14)address;
  switch(address) {
  case 0x8200 ... 0x83ff: return readColor(address);  //K2GE only
  case 0x8800 ... 0x88ff: return readObject(address);
  case 0x8c00 ... 0x8c3f: return readObjectColor(address);  //K2GE only
  case 0x9000 ... 0x9fff: return readAttribute(address);
  case 0xa000 ... 0xbfff: return readCharacter(address);
  }

  n8 data;

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
    data.bit(7)   = dac.negate;
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
    data.bit(0,6) = 0;
    data.bit(7)   = dac.colorMode;
    break;

  case 0x87fe: data = 0x3f; break;  //input port register (reserved)
  }

  return data;
}

auto KGE::write(n24 address, n8 data) -> void {
  address = 0x8000 | (n14)address;
  switch(address) {
  case 0x8200 ... 0x83ff: return writeColor(address, data);  //K2GE only
  case 0x8800 ... 0x88ff: return writeObject(address, data);
  case 0x8c00 ... 0x8cff: return writeObjectColor(address, data);  //K2GE only
  case 0x9000 ... 0x9fff: return writeAttribute(address, data);
  case 0xa000 ... 0xbfff: return writeCharacter(address, data);
  }

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
    dac.negate   = data.bit(7);
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

auto KGE::readObject(n8 address) -> n8 {
  n8 data;
  auto& object = sprite.objects[address >> 2];
  switch(address & 3) {
  case 0:
    data = object.character.bit(0,7);
    break;
  case 1:
    data.bit(0)   = object.character.bit(8);
    data.bit(1)   = object.vchain;
    data.bit(2)   = object.hchain;
    data.bit(3,4) = object.priority;
    data.bit(5)   = object.palette;
    data.bit(6)   = object.vflip;
    data.bit(7)   = object.hflip;
    break;
  case 2:
    data = object.hoffset;
    break;
  case 3:
    data = object.voffset;
    break;
  }
  return data;
}

auto KGE::writeObject(n8 address, n8 data) -> void {
  auto& object = sprite.objects[address >> 2];
  switch(address & 3) {
  case 0:
    object.character.bit(0,7) = data;
    break;
  case 1:
    object.character.bit(8) = data.bit(0);
    object.vchain           = data.bit(1);
    object.hchain           = data.bit(2);
    object.priority         = data.bit(3,4);
    object.palette          = data.bit(5);
    object.vflip            = data.bit(6);
    object.hflip            = data.bit(7);
    break;
  case 2:
    object.hoffset = data;
    break;
  case 3:
    object.voffset = data;
    break;
  }
}

auto KGE::readObjectColor(n6 address) -> n8 {
  if(!Model::NeoGeoPocketColor()) return 0x00;
  return sprite.objects[address].code;  //d4-d7 = 0
}

auto KGE::writeObjectColor(n6 address, n8 data) -> void {
  if(!Model::NeoGeoPocketColor()) return;
  sprite.objects[address].code = data.bit(0,3);
}

auto KGE::readColor(n9 address) -> n8 {
  if(!Model::NeoGeoPocketColor()) return 0x00;
  n8 data;
  auto& color = dac.colors[address >> 1];
  switch(address & 1) {
  case 0:
    data = color.bit(0,7);
    break;
  case 1:
    data.bit(0,3) = color.bit(8,11);
    data.bit(4,7) = 0;
    break;
  }
  return data;
}

auto KGE::writeColor(n9 address, n8 data) -> void {
  if(!Model::NeoGeoPocketColor()) return;
  auto& color = dac.colors[address >> 1];
  switch(address & 1) {
  case 0:
    color.bit(0, 7) = data.bit(0,7);
    break;
  case 1:
    color.bit(8,11) = data.bit(0,3);
    break;
  }
}

auto KGE::readAttribute(n12 address) -> n8 {
  n8 data;
  auto& attribute = attributes[address >> 1];
  switch(address & 1) {
  case 0:
    data = attribute.character.bit(0,7);
    break;
  case 1:
    data.bit(0)   = attribute.character.bit(8);
    data.bit(1,4) = attribute.code;
    data.bit(5)   = attribute.palette;
    data.bit(6)   = attribute.vflip;
    data.bit(7)   = attribute.hflip;
    break;
  }
  return data;
}

auto KGE::writeAttribute(n12 address, n8 data) -> void {
  auto& attribute = attributes[address >> 1];
  switch(address & 1) {
  case 0:
    attribute.character.bit(0,7) = data.bit(0,7);
    break;
  case 1:
    attribute.character.bit(8) = data.bit(0);
    attribute.code             = data.bit(1,4);
    attribute.palette          = data.bit(5);
    attribute.vflip            = data.bit(6);
    attribute.hflip            = data.bit(7);
    break;
  }
}

auto KGE::readCharacter(n13 address) -> n8 {
  n8 data;
  auto& character = characters[address >> 4];
  n3 y = address >> 1;
  switch(address & 1) {
  case 0:
    data.bit(0,1) = character[y][7];
    data.bit(2,3) = character[y][6];
    data.bit(4,5) = character[y][5];
    data.bit(6,7) = character[y][4];
    break;
  case 1:
    data.bit(0,1) = character[y][3];
    data.bit(2,3) = character[y][2];
    data.bit(4,5) = character[y][1];
    data.bit(6,7) = character[y][0];
    break;
  }
  return data;
}

auto KGE::writeCharacter(n13 address, n8 data) -> void {
  auto& character = characters[address >> 4];
  n3 y = address >> 1;
  switch(address & 1) {
  case 0:
    character[y][7] = data.bit(0,1);
    character[y][6] = data.bit(2,3);
    character[y][5] = data.bit(4,5);
    character[y][4] = data.bit(6,7);
    break;
  case 1:
    character[y][3] = data.bit(0,1);
    character[y][2] = data.bit(2,3);
    character[y][1] = data.bit(4,5);
    character[y][0] = data.bit(6,7);
    break;
  }
}
