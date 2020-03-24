auto V9938::sprite1(uint8 voffset) -> void {
  sprite.collision = 0;
  sprite.overflow = 0;
  sprite.last = 0;

  uint3 valid = 0;
  uint5 sizeLimit = (8 << sprite.size << sprite.magnify) - 1;
  for(uint index : range(4)) sprites[index].y = 0xd0;

  uint14 address = table.spriteAttribute & 0x03f80;
  for(uint index : range(32)) {
    uint8 y = videoRAM.read(address++);
    if(y == 0xd0) break;

    uint8 x = videoRAM.read(address++);
    uint8 pattern = videoRAM.read(address++);
    uint8 attributes = videoRAM.read(address++);

    y += 1;
    if(voffset < y) continue;
    if(voffset > y + sizeLimit) continue;

    //16x16 sprites use four patterns; thus lower two pattern bits are ignored
    if(sprite.size) pattern.bit(0,1) = 0;

    if(valid == 4) {
      sprite.overflow = 1;
      sprite.last = index;
      break;
    }

    uint4 color = attributes.bit(0,3);
    if(attributes.bit(7)) x -= 32;

    sprites[valid++] = {x, y, pattern, color};
  }
}

auto V9938::sprite1(uint4& color, uint8 hoffset, uint8 voffset) -> void {
  uint4 output;
  uint5 sizeLimit = (8 << sprite.size << sprite.magnify) - 1;

  for(uint index : range(4)) {
    auto& o = sprites[index];
    if(o.y == 0xd0) break;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + sizeLimit) continue;

    uint4 x = hoffset - o.x >> sprite.magnify;
    uint4 y = voffset - o.y >> sprite.magnify;

    uint14 address = table.spritePatternGenerator;
    address += (o.pattern << 3) + (x >> 3 << 4) + y;

    if(videoRAM.read(address).bit(~x & 7)) {
      if(output) { sprite.collision = 1; break; }
      output = o.color;
    }
  }

  if(output) color = output;
}
