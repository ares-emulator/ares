auto V9938::sprite1(n8 voffset) -> void {
  sprite.collision = 0;
  sprite.overflow = 0;
  sprite.last = 0;

  n3 valid = 0;
  n5 sizeLimit = (8 << sprite.size << sprite.magnify) - 1;
  for(u32 index : range(4)) sprites[index].y = 0xd0;

  n14 address = table.spriteAttribute & 0x03f80;
  for(u32 index : range(32)) {
    n8 y = videoRAM.read(address++);
    if(y == 0xd0) break;

    n8 x = videoRAM.read(address++);
    n8 pattern = videoRAM.read(address++);
    n8 attributes = videoRAM.read(address++);

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

    n4 color = attributes.bit(0,3);
    if(attributes.bit(7)) x -= 32;

    sprites[valid++] = {x, y, pattern, color};
  }
}

auto V9938::sprite1(n4& color, n8 hoffset, n8 voffset) -> void {
  n4 output;
  n5 sizeLimit = (8 << sprite.size << sprite.magnify) - 1;

  for(u32 index : range(4)) {
    auto& o = sprites[index];
    if(o.y == 0xd0) break;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + sizeLimit) continue;

    n4 x = hoffset - o.x >> sprite.magnify;
    n4 y = voffset - o.y >> sprite.magnify;

    n14 address = table.spritePatternGenerator;
    address += (o.pattern << 3) + (x >> 3 << 4) + y;

    if(videoRAM.read(address).bit(~x & 7)) {
      if(output) { sprite.collision = 1; break; }
      output = o.color;
    }
  }

  if(output) color = output;
}
