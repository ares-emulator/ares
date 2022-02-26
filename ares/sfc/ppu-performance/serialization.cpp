auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  PPUcounter::serialize(s);

  s(ppu1.version);
  s(ppu1.mdr);

  s(ppu2.version);
  s(ppu2.mdr);

  s(array_span<n16>{vram.data, vram.mask + 1});
  s(vram.address);
  s(vram.increment);
  s(vram.mapping);
  s(vram.mode);

  s(state.interlace);
  s(state.overscan);
  s(state.vdisp);

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
  s(io.cgramAddress);
  s(io.cgramAddressLatch);
  s(io.interlace);
  s(io.overscan);
  s(io.pseudoHires);
  s(io.extbg);
  s(io.hcounter);
  s(io.vcounter);

  s(mode7.hoffset);
  s(mode7.voffset);
  s(mode7.hflip);
  s(mode7.vflip);
  s(mode7.repeat);
  s(mode7.a);
  s(mode7.b);
  s(mode7.c);
  s(mode7.d);
  s(mode7.hcenter);
  s(mode7.vcenter);

  s(window);
  s(mosaic);
  s(bg1);
  s(bg2);
  s(bg3);
  s(bg4);
  s(obj);
  s(dac);
}

auto PPU::Window::Layer::serialize(serializer& s) -> void {
  s(oneInvert);
  s(oneEnable);
  s(twoInvert);
  s(twoEnable);
  s(mask);
  s(aboveEnable);
  s(belowEnable);
}

auto PPU::Window::Color::serialize(serializer& s) -> void {
  s(oneInvert);
  s(oneEnable);
  s(twoInvert);
  s(twoEnable);
  s(mask);
  s(aboveMask);
  s(belowMask);
}

auto PPU::Window::serialize(serializer& s) -> void {
  s(io.oneLeft);
  s(io.oneRight);
  s(io.twoLeft);
  s(io.twoRight);
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
  s(io.hoffset);
  s(io.voffset);
  s(io.aboveEnable);
  s(io.belowEnable);
  s(io.mosaicEnable);
  s(io.mode);
  s(io.priority);
  s(window);
}

auto PPU::OAM::serialize(serializer& s) -> void {
  for(auto& object : objects) {
    s(object.x);
    s(object.y);
    s(object.character);
    s(object.nameselect);
    s(object.vflip);
    s(object.hflip);
    s(object.priority);
    s(object.palette);
    s(object.size);
  }
}

auto PPU::Object::serialize(serializer& s) -> void {
  s(oam);
  s(io.interlace);
  s(io.tiledataAddress);
  s(io.nameselect);
  s(io.baseSize);
  s(io.firstSprite);
  s(io.aboveEnable);
  s(io.belowEnable);
  s(io.rangeOver);
  s(io.timeOver);
  s(io.priority);
  s(window);
}

auto PPU::DAC::serialize(serializer& s) -> void {
  s(cgram);
  s(io.directColor);
  s(io.blendMode);
  s(io.colorEnable);
  s(io.colorHalve);
  s(io.colorMode);
  s(io.colorRed);
  s(io.colorGreen);
  s(io.colorBlue);
  s(window);
}
