auto TMS9918::serialize(serializer& s) -> void {
  s(vram);

  s(io.vcounter);
  s(io.hcounter);

  s(io.controlLatch);
  s(io.controlValue);
  s(io.vramLatch);

  s(io.spriteOverflowIndex);
  s(io.spriteCollision);
  s(io.spriteOverflow);
  s(io.irqLine);

  s(io.externalInput);
  s(io.videoMode);
  s(io.spriteZoom);
  s(io.spriteSize);
  s(io.irqEnable);
  s(io.displayEnable);
  s(io.ramMode);
  s(io.nameTableAddress);
  s(io.colorTableAddress);
  s(io.patternTableAddress);
  s(io.spriteAttributeTableAddress);
  s(io.spritePatternTableAddress);
  s(io.colorBackground);
  s(io.colorForeground);

  for(auto& sprite : sprites) {
    s(sprite.x);
    s(sprite.y);
    s(sprite.pattern);
    s(sprite.color);
  }

  s(output.color);
}
