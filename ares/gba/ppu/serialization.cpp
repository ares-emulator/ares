auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(vram);
  s(pram);

  s(io.gameBoyColorMode);
  s(io.forceBlank);
  s(io.greenSwap);
  s(io.vblank);
  s(io.hblank);
  s(io.vcoincidence);
  s(io.irqvblank);
  s(io.irqhblank);
  s(io.irqvcoincidence);
  s(io.vcompare);
  s(io.vcounter);

  s(Background::IO::mode);
  s(Background::IO::frame);
  s(Background::IO::mosaicWidth);
  s(Background::IO::mosaicHeight);
  s(bg0);
  s(bg1);
  s(bg2);
  s(bg3);
  s(objects);
  s(window0);
  s(window1);
  s(window2);
  s(window3);
  s(dac);
  for(auto& object : this->object) s(object);
  for(auto& param : this->objectParam) s(param);
}

auto PPU::Background::serialize(serializer& s) -> void {
  s(id);

  s(io.enable);
  s(io.priority);
  s(io.characterBase);
  s(io.unused);
  s(io.mosaic);
  s(io.colorMode);
  s(io.screenBase);
  s(io.affineWrap);
  s(io.screenSize);
  s(io.hoffset);
  s(io.voffset);
  s(io.pa);
  s(io.pb);
  s(io.pc);
  s(io.pd);
  s(io.x);
  s(io.y);
  s(io.lx);
  s(io.ly);

  s(mosaicOffset);
  s(hmosaic);
  s(vmosaic);
  s(fx);
  s(fy);
}

auto PPU::Objects::serialize(serializer& s) -> void {
  s(io.enable);
  s(io.hblank);
  s(io.mapping);
  s(io.mosaicWidth);
  s(io.mosaicHeight);

  s(mosaicOffset);
}

auto PPU::Window::serialize(serializer& s) -> void {
  s(id);

  s(io.enable);
  s(io.active);
  s(io.x1);
  s(io.x2);
  s(io.y1);
  s(io.y2);

  s(output);
}

auto PPU::DAC::serialize(serializer& s) -> void {
  s(io.blendMode);
  s(io.blendAbove);
  s(io.blendBelow);
  s(io.blendEVA);
  s(io.blendEVB);
  s(io.blendEVY);
}

auto PPU::Object::serialize(serializer& s) -> void {
  s(y);
  s(affine);
  s(affineSize);
  s(mode);
  s(mosaic);
  s(colors);
  s(shape);
  s(x);
  s(affineParam);
  s(hflip);
  s(vflip);
  s(size);
  s(character);
  s(priority);
  s(palette);
  s(width);
  s(height);
}

auto PPU::ObjectParam::serialize(serializer& s) -> void {
  s(pa);
  s(pb);
  s(pc);
  s(pd);
}
