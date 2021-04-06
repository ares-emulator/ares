inline auto VDP::Object::width() const -> u32 {
  return 1 + tileWidth << 3;
}

inline auto VDP::Object::height() const -> u32 {
  return 1 + tileHeight << 3 + (vdp.io.interlaceMode == 3);
}

auto VDP::Sprite::write(n9 address, n16 data) -> void {
  if(address > 320) return;

  auto& object = oam[address >> 2];
  switch(address.bit(0,1)) {
  case 0:
    object.y = data.bit(0,9);
    break;
  case 1:
    object.link       = data.bit(0,6);
    object.tileHeight = data.bit(8,9);
    object.tileWidth  = data.bit(10,11);
    break;
  case 2:
    object.address  = data.bit(0,10);
    object.hflip    = data.bit(11);
    object.vflip    = data.bit(12);
    object.palette  = data.bit(13,14);
    object.priority = data.bit(15);
    break;
  case 3:
    object.x = data.bit(0,8);
    break;
  }
}

auto VDP::Sprite::mappingFetch() -> void {
}

auto VDP::Sprite::patternFetch() -> void {
}

auto VDP::Sprite::scanline(u32 y) -> void {
  bool interlace = vdp.io.interlaceMode == 3;
  y += 128;
  if(interlace) y = y << 1 | vdp.state.field;

  objects.reset();
  n7  link  = 0;
  u32 tiles = 0;
  u32 count = 0;

  do {
    auto& object = oam[link];
    link = object.link;

    if(y <  object.y) continue;
    if(y >= object.y + object.height()) continue;
    if(object.x == 0) break;

    if(objects.size() >= objectLimit() || tiles >= tileLimit()) {
      overflow = 1;
      break;
    }

    objects.append(object);
    tiles += object.width() >> 3;
    if(!link || link >= linkLimit()) break;
  } while(++count < linkLimit());
}

auto VDP::Sprite::run(u32 x, u32 y) -> void {
  bool interlace = vdp.io.interlaceMode == 3;
  x += 128;
  y += 128;
  if(interlace) y = y << 1 | vdp.state.field;

  output.priority = 0;
  output.color = 0;

  for(auto& object : objects) {
    if(x <  object.x) continue;
    if(x >= object.x + object.width()) continue;

    u32 objectX = x - object.x;
    u32 objectY = y - object.y;
    if(object.hflip) objectX = (object.width() - 1) - objectX;
    if(object.vflip) objectY = (object.height() - 1) - objectY;

    u32 tileX = objectX >> 3;
    u32 tileY = objectY >> 3 + interlace;
    u32 tileNumber = tileX * (object.height() >> 3 + interlace) + tileY;
    n15 tileAddress = object.address + tileNumber << 4 + interlace;
    u32 pixelX = objectX & 7;
    u32 pixelY = objectY & 7 + interlace * 8;
    tileAddress += pixelY << 1 | pixelX >> 2;

    n16 tileData = vdp.vram.read(generatorAddress | tileAddress);
    n4  color = tileData >> (((pixelX & 3) ^ 3) << 2);
    if(!color) continue;

    if(output.color) {
      collision = 1;
    }

    output.color = object.palette << 4 | color;
    output.priority = object.priority;
    break;
  }
}

auto VDP::Sprite::power(bool reset) -> void {
  generatorAddress = 0;
  nametableAddress = 0;
  collision = 0;
  overflow = 0;
  output = {};
}
