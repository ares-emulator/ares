auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(vram);
  s(cram);
  s(background);
  s(sprite);
  s(dac);
  s(irq.line.enable);
  s(irq.line.pending);
  s(irq.line.counter);
  s(irq.frame.enable);
  s(irq.frame.pending);
  s(io.mode);
  s(io.vcounter);
  s(io.hcounter);
  s(io.pcounter);
  s(io.lcounter);
  s(io.ccounter);
  s(io.controlLatch);
  s(io.controlData);
  s(io.code);
  s(io.address);
  s(io.vramLatch);
}

auto VDP::Background::serialize(serializer& s) -> void {
  s(io.hscrollLock);
  s(io.vscrollLock);
  s(io.nameTableAddress);
  s(io.colorTableAddress);
  s(io.patternTableAddress);
  s(io.hscroll);
  s(io.vscroll);
  s(latch.nameTableAddress);
  s(latch.hscroll);
  s(latch.vscroll);
  s(output.color);
  s(output.palette);
  s(output.priority);
}

auto VDP::Sprite::serialize(serializer& s) -> void {
  s(io.zoom);
  s(io.size);
  s(io.shift);
  s(io.attributeTableAddress);
  s(io.patternTableAddress);
  s(io.overflow);
  s(io.collision);
  s(io.fifth);
  s(output.color);
  for(auto& object : objects) {
    s(object.x);
    s(object.y);
    s(object.pattern);
    s(object.color);
  }
  s(objectsValid);
}

auto VDP::DAC::serialize(serializer& s) -> void {
  s(io.displayEnable);
  s(io.externalSync);
  s(io.leftClip);
  s(io.backdropColor);
}
