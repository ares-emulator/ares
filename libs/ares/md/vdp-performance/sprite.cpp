auto VDP::Sprite::render() -> void {
  bool interlace = vdp.io.interlaceMode == 3;
  u32 y = vdp.state.vcounter + 128;
  if(interlace) y = y << 1 | vdp.state.field;

  u32 link = 0;
  u32 tiles = 0;
  u32 count = 0;
  u32 objectSize = 0;

  do {
    auto& object = oam[link];
    link = object.link;

    auto objectY = object.y & (interlace ? 1023 : 511);
    if(y <  objectY) continue;
    if(y >= objectY + object.height()) continue;
    if(object.x == 0) break;

    objects[objectSize++] = object;
    tiles += object.width() >> 3;

    if(!link || link >= linkLimit()) break;
    if(objectSize >= objectLimit()) break;
    if(tiles >= tileLimit()) break;
  } while(++count < linkLimit());

  memory::fill<Pixel>(pixels, vdp.screenWidth());
  u32 shiftY = interlace ? 4 : 3;
  u32 maskY = interlace ? 15 : 7;
  u32 tileShift = interlace ? 7 : 6;

  for(s32 index = objectSize - 1; index >= 0; index--) {
    auto& object = objects[index];
    u32 objectY = y - (object.y & (interlace ? 1023 : 511));
    if(object.verticalFlip) objectY = (object.height() - 1) - objectY;
    u32 tileIncrement = (object.height() >> interlace) >> 3 << tileShift;
    u32 tileAddress = object.address + (objectY >> shiftY) << tileShift;
    tileAddress += (objectY & maskY) << 3;
    auto tileData = &vdp.vram.pixels[tileAddress & 0x1fff8];
    u32 w = !object.horizontalFlip ? object.x - 128 : (object.x + object.width() - 1) - 128;
    s32 incrementX = object.horizontalFlip ? -1 : +1;
    for(u32 objectX = 0; objectX < object.width();) {
      if(u32 color = tileData[objectX & 7]) {
        pixels[w & 511].color = object.palette << 4 | color;
        pixels[w & 511].priority = object.priority;
      }
      w += incrementX;
      if((objectX++ & 7) == 7) {
        tileAddress += tileIncrement;
        tileData = &vdp.vram.pixels[tileAddress & 0x1fff8];
      }
    }
  }
}

auto VDP::Sprite::write(n9 address, n16 data) -> void {
  if(address > 320) return;

  auto& object = oam[address >> 2];
  switch(address.bit(0,1)) {
  case 0:
    object.y = data.bit(0,9);
    break;
  case 1:
    object.link = data.bit(0,6);
    object.tileHeight = data.bit(8,9);
    object.tileWidth = data.bit(10,11);
    break;
  case 2:
    object.address = data.bit(0,10);
    object.horizontalFlip = data.bit(11);
    object.verticalFlip = data.bit(12);
    object.palette = data.bit(13,14);
    object.priority = data.bit(15);
    break;
  case 3:
    object.x = data.bit(0,8);
    break;
  }
}
