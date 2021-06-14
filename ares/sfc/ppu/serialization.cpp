auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  PPUcounter::serialize(s);

  s(self.interlace);
  s(self.overscan);
  s(self.vdisp);

  s(array_span<u16>{vram.data, vram.mask + 1});
  s(oam.object);
  s(cgram);

  s(ppu1.version);
  s(ppu1.mdr);

  s(ppu2.version);
  s(ppu2.mdr);

  s(latch.vram);
  s(latch.oam);
  s(latch.cgram);
  s(latch.bgofsPPU1);
  s(latch.bgofsPPU2);
  s(latch.mode7);
  s(latch.counters);
  s(latch.hcounter);
  s(latch.vcounter);

  s(latch.oamAddress);
  s(latch.cgramAddress);

  s(io.displayBrightness);
  s(io.displayDisable);

  s(io.oamBaseAddress);
  s(io.oamAddress);
  s(io.oamPriority);

  s(io.bgMode);
  s(io.bgPriority);

  s(io.hoffsetMode7);
  s(io.voffsetMode7);

  s(io.vramIncrementSize);
  s(io.vramMapping);
  s(io.vramIncrementMode);

  s(io.vramAddress);

  s(io.hflipMode7);
  s(io.vflipMode7);
  s(io.repeatMode7);

  s(io.m7a);
  s(io.m7b);
  s(io.m7c);
  s(io.m7d);
  s(io.m7x);
  s(io.m7y);

  s(io.cgramAddress);
  s(io.cgramAddressLatch);

  s(io.interlace);
  s(io.overscan);
  s(io.pseudoHires);
  s(io.extbg);

  s(io.hcounter);
  s(io.vcounter);

  s(mosaic);
  s(bg1);
  s(bg2);
  s(bg3);
  s(bg4);
  s(obj);
  s(window);
  s(dac);
}

auto PPU::OAM::Object::serialize(serializer& s) -> void {
  s(x);
  s(y);
  s(character);
  s(nameselect);
  s(vflip);
  s(hflip);
  s(priority);
  s(palette);
  s(size);
}

auto PPU::Mosaic::serialize(serializer& s) -> void {
  s(size);
  s(vcounter);
}

auto PPU::Background::serialize(serializer& s) -> void {
  s(io.screenSize);
  s(io.screenAddress);
  s(io.tiledataAddress);
  s(io.tileSize);
  s(io.mode);
  s(io.priority);
  s(io.aboveEnable);
  s(io.belowEnable);
  s(io.hoffset);
  s(io.voffset);

  s(output.above.priority);
  s(output.above.palette);
  s(output.above.paletteGroup);

  s(output.below.priority);
  s(output.below.palette);
  s(output.below.paletteGroup);

  s(mosaic.enable);
  s(mosaic.hcounter);
  s(mosaic.hoffset);

  s(mosaic.pixel.priority);
  s(mosaic.pixel.palette);
  s(mosaic.pixel.paletteGroup);

  s(opt.hoffset);
  s(opt.voffset);

  for(auto& tile : tiles) {
    s(tile.address);
    s(tile.character);
    s(tile.palette);
    s(tile.paletteGroup);
    s(tile.priority);
    s(tile.hmirror);
    s(tile.vmirror);
    s(tile.data);
  }

  s(renderingIndex);
  s(pixelCounter);
}

auto PPU::Object::serialize(serializer& s) -> void {
  s(io.aboveEnable);
  s(io.belowEnable);
  s(io.interlace);

  s(io.tiledataAddress);
  s(io.nameselect);
  s(io.baseSize);
  s(io.firstSprite);

  s(io.priority);

  s(io.rangeOver);
  s(io.timeOver);

  s(latch.firstSprite);

  s(t.x);
  s(t.y);

  s(t.itemCount);
  s(t.tileCount);

  s(t.active);
  for(u32 p : range(2)) {
    for(u32 n : range(32)) {
      s(t.item[p][n].valid);
      s(t.item[p][n].index);
    }
    for(u32 n : range(34)) {
      s(t.tile[p][n].valid);
      s(t.tile[p][n].x);
      s(t.tile[p][n].priority);
      s(t.tile[p][n].palette);
      s(t.tile[p][n].hflip);
      s(t.tile[p][n].data);
    }
  }

  s(output.above.priority);
  s(output.above.palette);

  s(output.below.priority);
  s(output.below.palette);
}

auto PPU::Window::serialize(serializer& s) -> void {
  s(io.bg1.oneInvert);
  s(io.bg1.oneEnable);
  s(io.bg1.twoInvert);
  s(io.bg1.twoEnable);
  s(io.bg1.mask);
  s(io.bg1.aboveEnable);
  s(io.bg1.belowEnable);

  s(io.bg2.oneEnable);
  s(io.bg2.oneInvert);
  s(io.bg2.twoEnable);
  s(io.bg2.twoInvert);
  s(io.bg2.mask);
  s(io.bg2.aboveEnable);
  s(io.bg2.belowEnable);

  s(io.bg3.oneEnable);
  s(io.bg3.oneInvert);
  s(io.bg3.twoEnable);
  s(io.bg3.twoInvert);
  s(io.bg3.mask);
  s(io.bg3.aboveEnable);
  s(io.bg3.belowEnable);

  s(io.bg4.oneEnable);
  s(io.bg4.oneInvert);
  s(io.bg4.twoEnable);
  s(io.bg4.twoInvert);
  s(io.bg4.mask);
  s(io.bg4.aboveEnable);
  s(io.bg4.belowEnable);

  s(io.obj.oneEnable);
  s(io.obj.oneInvert);
  s(io.obj.twoEnable);
  s(io.obj.twoInvert);
  s(io.obj.mask);
  s(io.obj.aboveEnable);
  s(io.obj.belowEnable);

  s(io.col.oneEnable);
  s(io.col.oneInvert);
  s(io.col.twoEnable);
  s(io.col.twoInvert);
  s(io.col.mask);
  s(io.col.aboveMask);
  s(io.col.belowMask);

  s(io.oneLeft);
  s(io.oneRight);
  s(io.twoLeft);
  s(io.twoRight);

  s(output.above.colorEnable);
  s(output.below.colorEnable);

  s(x);
}

auto PPU::DAC::serialize(serializer& s) -> void {
  s(io.directColor);
  s(io.blendMode);

  s(io.bg1.colorEnable);
  s(io.bg2.colorEnable);
  s(io.bg3.colorEnable);
  s(io.bg4.colorEnable);
  s(io.obj.colorEnable);
  s(io.back.colorEnable);
  s(io.colorHalve);
  s(io.colorMode);

  s(io.colorRed);
  s(io.colorGreen);
  s(io.colorBlue);

  s(math.above.color);
  s(math.above.colorEnable);
  s(math.below.color);
  s(math.below.colorEnable);
  s(math.transparent);
  s(math.blendMode);
  s(math.colorHalve);
}
