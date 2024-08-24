auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("PPU VRAM");
  memory.vram->setSize(ppu.vram.size());
  memory.vram->setRead([&](u32 address) -> u8 {
    return ppu.vram[address];
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    ppu.vram[address] = data;
  });

  memory.pram = parent->append<Node::Debugger::Memory>("PPU PRAM");
  memory.pram->setSize(ppu.pram.size());
  memory.pram->setRead([&](u32 address) -> u8 {
    return ppu.pram[address >> 1].byte(address & 1);
  });
  memory.pram->setWrite([&](u32 address, u8 data) -> void {
    ppu.pram[address >> 1].byte(address & 1) = data;
  });

  graphics.tiles4bpp = parent->append<Node::Debugger::Graphics>("4 BPP Tiles");
  graphics.tiles4bpp->setSize(512, 384);
  graphics.tiles4bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(512 * 384);
    for(u32 tileY : range(48)) {
      for(u32 tileX : range(64)) {
        n32 address = tileY * 64 + tileX << 5;
        for(u32 y : range(8)) {
          for(u32 x : range(4)) {
            n8 colors = ppu.vram[address + y * 4 + x];
            n4 pixel1 = ppu.pram[colors & 0xf];
            n4 pixel2 = ppu.pram[colors >> 4];
            output[(tileY * 8 + y) * 512 + (tileX * 8 + (x << 1))]     = pixel1 * 0x111111;
            output[(tileY * 8 + y) * 512 + (tileX * 8 + (x << 1)) + 1] = pixel2 * 0x111111;
          }
        }
      }
    }
    return output;
  });

  graphics.tiles8bpp = parent->append<Node::Debugger::Graphics>("8 BPP Tiles");
  graphics.tiles8bpp->setSize(256, 384);
  graphics.tiles8bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(256 * 384);
    for(u32 tileY : range(48)) {
      for(u32 tileX : range(32)) {
        n32 address = tileY * 32 + tileX << 6;
        for(u32 y : range(8)) {
          for(u32 x : range(8)) {
            n8 color = ppu.vram[address + y * 8 + x];
            n15 pixel = ppu.pram[color];
            n8 r = pixel >>  0 & 31; r = r << 3 | r >> 2;
            n8 g = pixel >>  5 & 31; g = g << 3 | g >> 2;
            n8 b = pixel >> 10 & 31; b = b << 3 | b >> 2;
            n8 a = 255;
            output[(tileY * 8 + y) * 256 + (tileX * 8 + x)] = a << 24 | r << 16 | g << 8 | b << 0;
          }
        }
      }
    }
    return output;
  });

  graphics.mode3 = parent->append<Node::Debugger::Graphics>("Mode 3");
  graphics.mode3->setSize(240, 160);
  graphics.mode3->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(240 * 160);
    for(u32 y : range(160)) {
      for(u32 x : range(240)) {
        n15 pixel = ppu.vram[y * 480 + x * 2 + 1] << 8 | ppu.vram[y * 480 + x * 2] << 0;
        n8 r = pixel >>  0 & 31; r = r << 3 | r >> 2;
        n8 g = pixel >>  5 & 31; g = g << 3 | g >> 2;
        n8 b = pixel >> 10 & 31; b = b << 3 | b >> 2;
        n8 a = 255;
        output[y * 240 + x] = a << 24 | r << 16 | g << 8 | b << 0;
      }
    }
    return output;
  });

  graphics.mode4 = parent->append<Node::Debugger::Graphics>("Mode 4");
  graphics.mode4->setSize(240, 320);
  graphics.mode4->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(240 * 320);
    for(u32 y : range(160)) {
      for(u32 x : range(240)) {
        n8 color = ppu.vram[y * 240 + x];
        n15 pixel = ppu.pram[color];
        n8 r = pixel >>  0 & 31; r = r << 3 | r >> 2;
        n8 g = pixel >>  5 & 31; g = g << 3 | g >> 2;
        n8 b = pixel >> 10 & 31; b = b << 3 | b >> 2;
        n8 a = 255;
        output[y * 240 + x] = a << 24 | r << 16 | g << 8 | b << 0;
      }
    }
    for(u32 y : range(160)) {
      for(u32 x : range(240)) {
        n8 color = ppu.vram[0xa000 + y * 240 + x];
        n15 pixel = ppu.pram[color];
        n8 r = pixel >>  0 & 31; r = r << 3 | r >> 2;
        n8 g = pixel >>  5 & 31; g = g << 3 | g >> 2;
        n8 b = pixel >> 10 & 31; b = b << 3 | b >> 2;
        n8 a = 255;
        output[(y + 160) * 240 + x] = a << 24 | r << 16 | g << 8 | b << 0;
      }
    }
    return output;
  });

  graphics.mode5 = parent->append<Node::Debugger::Graphics>("Mode 5");
  graphics.mode5->setSize(160, 256);
  graphics.mode5->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(160 * 256);
    for(u32 y : range(256)) {
      for(u32 x : range(160)) {
        n15 pixel = ppu.vram[y * 320 + x * 2 + 1] << 8 | ppu.vram[y * 320 + x * 2] << 0;
        n8 r = pixel >>  0 & 31; r = r << 3 | r >> 2;
        n8 g = pixel >>  5 & 31; g = g << 3 | g >> 2;
        n8 b = pixel >> 10 & 31; b = b << 3 | b >> 2;
        n8 a = 255;
        output[y * 160 + x] = a << 24 | r << 16 | g << 8 | b << 0;
      }
    }
    return output;
  });
}

auto PPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.vram);
  parent->remove(memory.pram);
  parent->remove(graphics.tiles4bpp);
  parent->remove(graphics.tiles8bpp);
  parent->remove(graphics.mode3);
  parent->remove(graphics.mode4);
  parent->remove(graphics.mode5);
  memory.vram.reset();
  memory.pram.reset();
  graphics.tiles4bpp.reset();
  graphics.tiles8bpp.reset();
  graphics.mode3.reset();
  graphics.mode4.reset();
  graphics.mode5.reset();
}
