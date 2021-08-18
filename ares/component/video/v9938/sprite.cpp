auto V9938::Sprite::setup(n8 voffset) -> void {
  n4 valid = 0;
  n5 vlimit = (8 << io.zoom << io.size) - 1;

  switch(self.videoMode()) {

  case 0b00000:
  case 0b00010:
  case 0b00100: {
    for(auto& object : objects) object.y = 0xd0;

    n14 address = io.nameTableAddress & 0x03f80;
    for(u32 index : range(32)) {
      i9 y = self.vram.read(address++);
      if(y == 0xd0) break;
      if(y >= 0xe0) y -= 0x100;

      i9 x = self.vram.read(address++);
      n8 pattern = self.vram.read(address++);
      n8 attributes = self.vram.read(address++);

      y += 1;
      if(voffset < y) continue;
      if(voffset > y + vlimit) continue;

      //16x16 sprites use four patterns; thus lower two pattern bits are ignored
      if(io.size) pattern.bit(0,1) = 0;

      if(valid == 4) {
        io.overflow = 1;
        io.overflowIndex = index;
        break;
      }

      n4 color = attributes.bit(0,3);
      if(attributes.bit(7)) x -= 32;

      objects[valid++] = {x, y, pattern, color};
    }
  }

  case 0b01000:
  case 0b01100:
  case 0b10000:
  case 0b10100:
  case 0b11100: {
    for(auto& object : objects) object.y = 0xd8;
    if(io.disable) return;

    n17 address = io.nameTableAddress & 0x1fe00;
    for(u32 index : range(32)) {
      i9 y = self.vram.read(address++);
      if(y == 0xd8) break;
      if(y >= 0xe0) y -= 0x100;

      i9 x = self.vram.read(address++);
      n8 pattern = self.vram.read(address++);
      n8 reserved = self.vram.read(address++);

      y += 1;
      if(voffset < y) continue;
      if(voffset > y + vlimit) continue;

      //16x16 sprites use four patterns; thus lower two pattern bits are ignored
      if(io.size) pattern.bit(0,1) = 0;

      if(valid == 8) {
        io.overflow = 1;
        io.overflowIndex = index;
        break;
      }

      n17 colorTable = io.nameTableAddress & 0x1fc00;
      colorTable += index << 4;
      colorTable += voffset - y;

      n8 attributes = self.vram.read(colorTable);
      n4 color = attributes.bit(0,3);
      n1 collision = attributes.bit(5);
      n1 priority = attributes.bit(6);
      if(attributes.bit(7)) x -= 32;

      objects[valid++] = {x, y, pattern, color, collision, priority};
    }
  }

  }
}

auto V9938::Sprite::run(n8 x, n8 y) -> void {
  output = {};
  switch(self.videoMode()) {
  case 0b00000: return sprite1(x, y);
  case 0b00010: return sprite1(x, y);
  case 0b00100: return sprite1(x, y);
  case 0b01000: return sprite2(x, y);
  case 0b01100: return sprite2(x, y);
  case 0b10000: return sprite2(x, y);
  case 0b10100: return sprite2(x, y);
  case 0b11100: return sprite2(x, y);
  }
}

auto V9938::Sprite::sprite1(n8 hoffset, n8 voffset) -> void {
  n4 color;
  n5 hlimit = (8 << io.zoom << io.size) - 1, vlimit = hlimit;

  for(auto& o : objects) {
    if(o.y == 0xd0) break;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + hlimit) continue;

    n4 x = hoffset - o.x >> io.zoom;
    n4 y = voffset - o.y >> io.zoom;

    n14 address = io.patternTableAddress;
    address += (o.pattern << 3) + (x >> 3 << 4) + (y & vlimit);

    if(self.vram.read(address).bit(~x & 7)) {
      if(color) { io.collision = 1; break; }
      color = o.color;
    }
  }

  if(color) output.color = color;
}

auto V9938::Sprite::sprite2(n8 hoffset, n8 voffset) -> void {
  n4 color;
  n5 hlimit = (8 << io.zoom << io.size) - 1, vlimit = hlimit;

  for(auto& o : objects) {
    if(o.y == 0xd8) break;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + hlimit) continue;

    n4 x = hoffset - o.x >> io.zoom;
    n4 y = voffset - o.y >> io.zoom;

    n17 address = io.patternTableAddress;
    address += (o.pattern << 3) + (x >> 3 << 4) + (y & vlimit);

    if(self.vram.read(address).bit(~x & 7)) {
      if(color) { io.collision = 1; break; }
      color = o.color;
    }
  }

  if(color) output.color = color;
}

auto V9938::Sprite::power() -> void {
  for(auto& object : objects) object = {};
  io = {};
  output = {};
}
