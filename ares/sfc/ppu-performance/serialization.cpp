auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  PPUcounter::serialize(s);

  s.integer(ppu1.version);
  s.integer(ppu1.mdr);

  s.integer(ppu2.version);
  s.integer(ppu2.mdr);

  s.array(vram.data);
  s.integer(vram.mask);
  s.integer(vram.address);
  s.integer(vram.increment);
  s.integer(vram.mapping);
  s.integer(vram.mode);

  s.integer(state.interlace);
  s.integer(state.overscan);
  s.integer(state.vdisp);

  s.integer(latch.vram);
  s.integer(latch.oam);
  s.integer(latch.cgram);
  s.integer(latch.bgofsPPU1);
  s.integer(latch.bgofsPPU2);
  s.integer(latch.mode7);
  s.integer(latch.counters);
  s.integer(latch.hcounter);
  s.integer(latch.vcounter);
  s.integer(latch.oamAddress);
  s.integer(latch.cgramAddress);

  s.integer(io.displayBrightness);
  s.integer(io.displayDisable);
  s.integer(io.oamBaseAddress);
  s.integer(io.oamAddress);
  s.integer(io.oamPriority);
  s.integer(io.bgMode);
  s.integer(io.bgPriority);
  s.integer(io.cgramAddress);
  s.integer(io.cgramAddressLatch);
  s.integer(io.interlace);
  s.integer(io.overscan);
  s.integer(io.pseudoHires);
  s.integer(io.extbg);
  s.integer(io.hcounter);
  s.integer(io.vcounter);

  s.integer(mode7.hoffset);
  s.integer(mode7.voffset);
  s.integer(mode7.hflip);
  s.integer(mode7.vflip);
  s.integer(mode7.repeat);
  s.integer(mode7.a);
  s.integer(mode7.b);
  s.integer(mode7.c);
  s.integer(mode7.d);
  s.integer(mode7.hcenter);
  s.integer(mode7.vcenter);

  window.serialize(s);
  mosaic.serialize(s);
  bg1.serialize(s);
  bg2.serialize(s);
  bg3.serialize(s);
  bg4.serialize(s);
  obj.serialize(s);
  dac.serialize(s);
}

auto PPU::Window::Layer::serialize(serializer& s) -> void {
  s.integer(oneInvert);
  s.integer(oneEnable);
  s.integer(twoInvert);
  s.integer(twoEnable);
  s.integer(mask);
  s.integer(aboveEnable);
  s.integer(belowEnable);
}

auto PPU::Window::Color::serialize(serializer& s) -> void {
  s.integer(oneInvert);
  s.integer(oneEnable);
  s.integer(twoInvert);
  s.integer(twoEnable);
  s.integer(mask);
  s.integer(aboveMask);
  s.integer(belowMask);
}

auto PPU::Window::serialize(serializer& s) -> void {
  s.integer(io.oneLeft);
  s.integer(io.oneRight);
  s.integer(io.twoLeft);
  s.integer(io.twoRight);
}

auto PPU::Mosaic::serialize(serializer& s) -> void {
  s.integer(size);
  s.integer(vcounter);
}

auto PPU::Background::serialize(serializer& s) -> void {
  s.integer(io.screenSize);
  s.integer(io.screenAddress);
  s.integer(io.tiledataAddress);
  s.integer(io.tileSize);
  s.integer(io.hoffset);
  s.integer(io.voffset);
  s.integer(io.aboveEnable);
  s.integer(io.belowEnable);
  s.integer(io.mosaicEnable);
  s.integer(io.mode);
  s.array(io.priority);
  window.serialize(s);
}

auto PPU::OAM::serialize(serializer& s) -> void {
  for(auto& object : objects) {
    s.integer(object.x);
    s.integer(object.y);
    s.integer(object.character);
    s.integer(object.nameselect);
    s.integer(object.vflip);
    s.integer(object.hflip);
    s.integer(object.priority);
    s.integer(object.palette);
    s.integer(object.size);
  }
}

auto PPU::Object::serialize(serializer& s) -> void {
  oam.serialize(s);

  s.integer(io.interlace);
  s.integer(io.tiledataAddress);
  s.integer(io.nameselect);
  s.integer(io.baseSize);
  s.integer(io.firstSprite);
  s.integer(io.aboveEnable);
  s.integer(io.belowEnable);
  s.integer(io.rangeOver);
  s.integer(io.timeOver);
  s.array(io.priority);

  window.serialize(s);
}

auto PPU::DAC::serialize(serializer& s) -> void {
  s.array(cgram);

  s.integer(io.directColor);
  s.integer(io.blendMode);
  s.array(io.colorEnable);
  s.integer(io.colorHalve);
  s.integer(io.colorMode);
  s.integer(io.colorRed);
  s.integer(io.colorGreen);
  s.integer(io.colorBlue);

  window.serialize(s);
}
