auto TMS9918::serialize(serializer& s) -> void {
  s(vram);
  s(background);
  s(sprite);
  s(dac);
  s(irqFrame.enable);
  s(irqFrame.pending);
  s(io.vcounter);
  s(io.hcounter);
  s(io.controlLatch);
  s(io.controlValue);
  s(io.vramLatch);
  s(io.videoMode);
  s(io.vramMode);
}

auto TMS9918::Background::serialize(serializer& s) -> void {
  s(io.nameTableAddress);
  s(io.colorTableAddress);
  s(io.patternTableAddress);
  s(output.color);
}

auto TMS9918::Sprite::serialize(serializer& s) -> void {
  for(auto& object : objects) {
    s(object.x);
    s(object.y);
    s(object.pattern);
    s(object.color);
  }
  s(io.zoom);
  s(io.size);
  s(io.attributeTableAddress);
  s(io.patternTableAddress);
  s(io.overflowIndex);
  s(io.overflow);
  s(io.collision);
  s(output.color);
}

auto TMS9918::DAC::serialize(serializer& s) -> void {
  s(io.displayEnable);
  s(io.externalSync);
  s(io.colorBackground);
  s(io.colorForeground);
}
