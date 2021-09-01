auto VDP::Sprite::setup(n9 voffset) -> void {
  if(!self.displayEnable()) return;
  n8 valid = 0;
  n5 vlimit = (8 << io.zoom << io.size) - 1;
  for(auto& object : objects) object.y = 0xd0;

  if(!self.videoMode().bit(3)) {
    n14 attributeAddress;
    attributeAddress.bit(7,13) = io.attributeTableAddress;
    for(u32 index : range(32)) {
      i9 y = self.vram[attributeAddress++];
      if(y == 0xd0) break;
      if(y >= 0xe0) y -= 0x100;

      i9 x = self.vram[attributeAddress++];
      n8 pattern = self.vram[attributeAddress++];
      n8 extra = self.vram[attributeAddress++];

      if(extra.bit(7)) x -= 32;
      y += 1;
      if(voffset < y) continue;
      if(voffset > y + vlimit) continue;

      if(vlimit == (io.zoom ? 31 : 15)) pattern.bit(0,1) = 0;

      if(valid == 4) {
        io.overflow = 1;
        io.overflowIndex = index;
        break;
      }

      objects[valid++] = {x, y, pattern, extra.bit(0,3)};
    }
  } else {
    n14 attributeAddress;
    attributeAddress.bit(8,13) = io.attributeTableAddress.bit(1,6);

    for(u32 index : range(64)) {
      i9 y = self.vram[attributeAddress + index];
      if(self.vlines() == 192 && y == 0xd0) break;
      if(y >= 0xf0) y -= 0x100;

      i9 x = self.vram[attributeAddress + 0x80 + (index << 1)];
      n8 pattern = self.vram[attributeAddress + 0x81 + (index << 1)];

      if(io.shift) x -= 8;
      y += 1;
      if(voffset < y) continue;
      if(voffset > y + vlimit) continue;

      if(vlimit == (io.zoom ? 31 : 15)) pattern.bit(0) = 0;

      if(valid == 8) {
        io.overflow = 1;
        io.overflowIndex = index;
        break;
      }

      objects[valid++] = {x, y, pattern};
    }
  }
}

auto VDP::Sprite::run(n8 hoffset, n9 voffset) -> void {
  output = {};
  if(!self.displayEnable()) return;
  switch(self.videoMode()) {
  case 0b0000: return graphics1(hoffset, voffset);
  case 0b0001: return;
  case 0b0010: return graphics2(hoffset, voffset);
  case 0b0011: return;
  case 0b0100: return;
  case 0b0101: return;
  case 0b0110: return;
  case 0b0111: return;
  case 0b1000: return graphics3(hoffset, voffset, 192);
  case 0b1001: return;
  case 0b1010: return graphics3(hoffset, voffset, 192);
  case 0b1011: return graphics3(hoffset, voffset, 224);
  case 0b1100: return graphics3(hoffset, voffset, 192);
  case 0b1101: return;
  case 0b1110: return graphics3(hoffset, voffset, 240);
  case 0b1111: return graphics3(hoffset, voffset, 192);
  }
}

auto VDP::Sprite::graphics1(n8 hoffset, n9 voffset) -> void {
  //todo: are sprites different in graphics mode 1?
  return graphics2(hoffset, voffset);
}

auto VDP::Sprite::graphics2(n8 hoffset, n9 voffset) -> void {
  n5 hlimit = (8 << io.zoom << io.size) - 1, vlimit = hlimit;
  for(auto& o : objects) {
    if(o.y == 0xd0) continue;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + hlimit) continue;

    u32 x = hoffset - o.x >> io.zoom;
    u32 y = voffset - o.y >> io.zoom;

    n14 address;
    address.bit( 0,10) = (o.pattern << 3) + (x >> 3 << 4) + (y & vlimit);
    address.bit(11,13) = io.patternTableAddress;

    n3 index = x ^ 7;
    if(self.vram[address].bit(index)) {
      if(output.color && self.displayEnable()) { io.collision = 1; break; }
      output.color = o.color;
    }
  }
}

auto VDP::Sprite::graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void {
  n4 hlimit = (8 << io.zoom) - 1;
  n5 vlimit = (8 << io.zoom << io.size) - 1;
  for(auto& o : objects) {
    if(o.y == 0xd0) continue;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + hlimit) continue;

    u32 x = hoffset - o.x >> io.zoom;
    u32 y = voffset - o.y >> io.zoom;

    n14 address;
    address.bit(2,12) = (o.pattern << 3) + (y & vlimit);
    address.bit  (13) = io.patternTableAddress.bit(2);

    n3 index = x ^ 7;
    n4 color;
    color.bit(0) = self.vram[address | 0].bit(index);
    color.bit(1) = self.vram[address | 1].bit(index);
    color.bit(2) = self.vram[address | 2].bit(index);
    color.bit(3) = self.vram[address | 3].bit(index);
    if(color == 0) continue;

    if(output.color && self.displayEnable()) { io.collision = 1; break; }
    output.color = color;
  }
}

auto VDP::Sprite::power() -> void {
  for(auto& object : objects) object = {};
  io = {};
  output = {};
}
