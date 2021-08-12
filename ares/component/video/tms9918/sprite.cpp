auto TMS9918::Sprite::setup(n8 voffset) -> void {
  n8 valid = 0;
  n5 vlimit = (8 << io.zoom << io.size) - 1;
  for(auto& object : objects) object.y = 0xd0;

  n14 attributeAddress;
  attributeAddress.bit(7,13) = io.attributeTableAddress;
  for(u32 index : range(32)) {
    i9 y = self.vram.read(attributeAddress++);
    if(y == 0xd0) break;
    if(y >= 0xe0) y -= 0x100;

    i9 x = self.vram.read(attributeAddress++);
    n8 pattern = self.vram.read(attributeAddress++);
    n8 extra = self.vram.read(attributeAddress++);

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
}

auto TMS9918::Sprite::run(n8 hoffset, n8 voffset) -> void {
  output = {};

  n4 color;
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
    if(self.vram.read(address).bit(index)) {
      if(color) { io.collision = 1; break; }
      color = o.color;
    }
  }

  if(color) output.color = color;
}

auto TMS9918::Sprite::power() -> void {
  for(auto& object : objects) object = {};
  io = {};
  output = {};
}
