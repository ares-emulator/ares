auto VDP::Sprite::setup(n9 voffset) -> void {
  objectsValid = 0;
  n5 limit = (8 << io.size << io.zoom) - 1;

  if(!vdp.io.mode.bit(3)) {
    n14 attributeAddress;
    attributeAddress.bit(7,13) = io.attributeTableAddress;
    for(u32 index : range(32)) {
      n8 y = vdp.vram[attributeAddress++];
      if(y == 0xd0) break;

      n8 x = vdp.vram[attributeAddress++];
      n8 pattern = vdp.vram[attributeAddress++];
      n8 extra = vdp.vram[attributeAddress++];

      if(extra.bit(7)) x -= 32;
      y += 1;
      if(voffset < y) continue;
      if(voffset > y + limit) continue;

      if(limit == 15) pattern.bit(0,1) = 0;

      if(objectsValid == 4) {
        io.overflow = 1;
        io.fifth = index;
        break;
      }

      objects[objectsValid++] = {x, y, pattern, extra.bit(0,3)};
    }
  } else {
    n14 attributeAddress;
    attributeAddress.bit(8,13) = io.attributeTableAddress.bit(1,6);
    for(u32 index : range(64)) {
      n8 y = vdp.vram[attributeAddress + index];
      n8 x = vdp.vram[attributeAddress + 0x80 + (index << 1)];
      n8 pattern = vdp.vram[attributeAddress + 0x81 + (index << 1)];
      if(vdp.vlines() == 192 && y == 0xd0) break;

      if(io.shift) x -= 8;
      y += 1;
      if(voffset < y) continue;
      if(voffset > y + limit) continue;

      if(limit == 15) pattern.bit(0) = 0;

      if(objectsValid == 8) {
        io.overflow = 1;
      //todo: set io.fifth to something here?
        break;
      }

      objects[objectsValid++] = {x, y, pattern};
    }
  }
}

auto VDP::Sprite::run(n8 hoffset, n9 voffset) -> void {
  output = {};
  switch(vdp.io.mode) {
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
  n5 limit = (8 << io.size << io.zoom) - 1;
  for(u32 objectIndex : range(objectsValid)) {
    auto& o = objects[objectIndex];
    if(hoffset < o.x) continue;
    if(hoffset > o.x + limit) continue;

    u32 x = hoffset - o.x >> io.zoom;
    u32 y = voffset - o.y >> io.zoom;

    n14 address;
    address.bit( 0,10) = (o.pattern << 3) + (x >> 3 << 4) + (y & limit);
    address.bit(11,13) = io.patternTableAddress;

    n3 index = x ^ 7;
    if(vdp.vram[address].bit(index)) {
      if(output.color && vdp.dac.io.displayEnable) { io.collision = 1; break; }
      output.color = o.color;
    }
  }
}

auto VDP::Sprite::graphics3(n8 hoffset, n9 voffset, u32 vlines) -> void {
  n5 limit = (8 << io.size << io.zoom) - 1;
  for(u32 objectIndex : range(objectsValid)) {
    auto& o = objects[objectIndex];
    if(hoffset < o.x) continue;
    if(hoffset > o.x + 7) continue;

    u32 x = hoffset - o.x >> io.zoom;
    u32 y = voffset - o.y >> io.zoom;

    n14 address;
    address.bit(2,12) = (o.pattern << 3) + (y & limit);
    address.bit  (13) = io.patternTableAddress.bit(2);

    n3 index = x ^ 7;
    n4 color;
    color.bit(0) = vdp.vram[address | 0].bit(index);
    color.bit(1) = vdp.vram[address | 1].bit(index);
    color.bit(2) = vdp.vram[address | 2].bit(index);
    color.bit(3) = vdp.vram[address | 3].bit(index);
    if(color == 0) continue;

    if(output.color && vdp.dac.io.displayEnable) { io.collision = 1; break; }
    output.color = color;
  }
}

auto VDP::Sprite::power() -> void {
  io = {};
  output = {};
  objectsValid = 0;
}
