auto TMS9918::serialize(serializer& s) -> void {
  s.array(vram.data(), vram.size());

  s.integer(io.vcounter);
  s.integer(io.hcounter);

  s.integer(io.controlLatch);
  s.integer(io.controlValue);
  s.integer(io.vramLatch);

  s.integer(io.spriteOverflowIndex);
  s.integer(io.spriteCollision);
  s.integer(io.spriteOverflow);
  s.integer(io.irqLine);

  s.integer(io.externalInput);
  s.integer(io.videoMode);
  s.integer(io.spriteZoom);
  s.integer(io.spriteSize);
  s.integer(io.irqEnable);
  s.integer(io.displayEnable);
  s.integer(io.ramMode);
  s.integer(io.nameTableAddress);
  s.integer(io.colorTableAddress);
  s.integer(io.patternTableAddress);
  s.integer(io.spriteAttributeTableAddress);
  s.integer(io.spritePatternTableAddress);
  s.integer(io.colorBackground);
  s.integer(io.colorForeground);

  for(auto& sprite : sprites) {
    s.integer(sprite.x);
    s.integer(sprite.y);
    s.integer(sprite.pattern);
    s.integer(sprite.color);
  }

  s.integer(output.color);
}
