auto VDP::Sprite::render() -> void {
  bool interlace = vdp.io.interlaceMode == 3;
  uint y = vdp.state.vcounter + 128;
  if(interlace) y = y << 1 | vdp.state.field;

  uint link = 0;
  uint tiles = 0;
  uint count = 0;
  uint objectSize = 0;
  do {
    auto& object = oam[link];
    link = object.link;

    if(y <  object.y) continue;
    if(y >= object.y + object.height()) continue;
    if(object.x == 0) break;

    objects[objectSize++] = object;
    tiles += object.width() >> 3;
  } while(link && link < 80 && objectSize < 20 && tiles < 40 && ++count < 80);

  memory::fill<uint8>(pixels, vdp.screenWidth());
  uint shiftY = interlace ? 4 : 3;
  uint maskY = interlace ? 15 : 7;
  uint tileShift = interlace ? 7 : 6;

  for(int index = objectSize - 1; index >= 0; index--) {
    auto& object = objects[index];
    uint objectY = y - object.y;
    if(object.verticalFlip) objectY = (object.height() - 1) - objectY;
    uint tileIncrement = (object.height() >> interlace) >> 3 << tileShift;
    uint tileAddress = object.address + (objectY >> shiftY) << tileShift;
    tileAddress += (objectY & maskY) << 3;
    auto tileData = &vdp.vram.pixels[tileAddress & 0x1fff8];
    uint w = !object.horizontalFlip ? object.x - 128 : (object.x + object.width() - 1) - 128;
    int incrementX = object.horizontalFlip ? -1 : +1;
    for(uint objectX = 0; objectX < object.width();) {
      if(uint color = tileData[objectX & 7]) {
        pixels[w & 511] = object.palette << 0 | object.priority << 2 | color;
      }
      w += incrementX;
      if((objectX++ & 7) == 7) {
        tileAddress += tileIncrement;
        tileData = &vdp.vram.pixels[tileAddress & 0x1fff8];
      }
    }
  }
}

auto VDP::Sprite::write(uint9 address, uint16 data) -> void {
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
