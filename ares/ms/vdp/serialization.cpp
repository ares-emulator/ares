auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(vram);
  s(cram);
  s(background);
  s(sprite);
  s(dac);
  s(irq);
  s(io.code);
  s(io.address);
  s(io.displayEnable);
  s(io.videoMode);
  s(io.vcounter);
  s(io.hcounter);
  s(io.ccounter);
  s(latch.control);
  s(latch.hcounter);
  s(latch.vram);
  s(latch.cram);
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
  for(auto& object : objects) {
    s(object.x);
    s(object.y);
    s(object.pattern);
    s(object.color);
  }
  s(io.zoom);
  s(io.size);
  s(io.shift);
  s(io.attributeTableAddress);
  s(io.patternTableAddress);
  s(io.overflowIndex);
  s(io.overflow);
  s(io.collision);
  s(output.color);
}

auto VDP::DAC::serialize(serializer& s) -> void {
  s(io.externalSync);
  s(io.leftClip);
  s(io.backdropColor);
}

auto VDP::IRQ::serialize(serializer& s) -> void {
  s(line.enable);
  s(line.pending);
  s(line.counter);
  s(line.coincidence);
  s(frame.enable);
  s(frame.pending);
}
