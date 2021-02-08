auto V9938::sprite2(n8 voffset) -> void {
  if(sprite.disable) return;

  sprite.collision = 0;
  sprite.overflow = 0;
  sprite.last = 0;

  n4 valid = 0;
  n5 sizeLimit = (8 << sprite.size << sprite.magnify) - 1;
  for(u32 index : range(8)) sprites[index].y = 0xd8;

  n17 address = table.spriteAttribute & 0x1fe00;
  for(u32 index : range(32)) {
    n8 y = videoRAM.read(address++);
    if(y == 0xd8) break;

    n8 x = videoRAM.read(address++);
    n8 pattern = videoRAM.read(address++);
    n8 reserved = videoRAM.read(address++);

    y += 1;
    if(voffset < y) continue;
    if(voffset > y + sizeLimit) continue;

    //16x16 sprites use four patterns; thus lower two pattern bits are ignored
    if(sprite.size) pattern.bit(0,1) = 0;

    if(valid == 8) {
      sprite.overflow = 1;
      sprite.last = index;
      break;
    }

    n17 colorTable = table.spriteAttribute & 0x1fc00;
    colorTable += index << 4;
    colorTable += voffset - y;

    n8 attributes = videoRAM.read(colorTable);
    n4 color = attributes.bit(0,3);
    n1 collision = attributes.bit(5);
    n1 priority = attributes.bit(6);
    if(attributes.bit(7)) x -= 32;

    sprites[valid++] = {x, y, pattern, color, collision, priority};
  }
}

auto V9938::sprite2(n4& color, n8 hoffset, n8 voffset) -> void {
  if(sprite.disable) return;

  n4 output;
  n5 sizeLimit = (8 << sprite.size << sprite.magnify) - 1;

  for(u32 index : range(8)) {
    auto& o = sprites[index];
    if(o.y == 0xd8) break;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + sizeLimit) continue;

    n4 x = hoffset - o.x >> sprite.magnify;
    n4 y = voffset - o.y >> sprite.magnify;

    n17 address = table.spritePatternGenerator;
    address += (o.pattern << 3) + (x >> 3 << 4) + y;

    if(videoRAM.read(address).bit(~x & 7)) {
      if(output) { sprite.collision = 1; break; }
      output = o.color;
    }
  }

  if(output) color = output;
}
