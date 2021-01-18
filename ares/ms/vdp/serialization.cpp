auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(vram);
  s(cram);
  s(background);
  s(sprite);
  s(io.vcounter);
  s(io.hcounter);
  s(io.pcounter);
  s(io.lcounter);
  s(io.ccounter);
  s(io.intLine);
  s(io.intFrame);
  s(io.spriteOverflow);
  s(io.spriteCollision);
  s(io.fifthSprite);
  s(io.controlLatch);
  s(io.controlData);
  s(io.code);
  s(io.address);
  s(io.vramLatch);
  s(io.externalSync);
  s(io.spriteShift);
  s(io.lineInterrupts);
  s(io.leftClip);
  s(io.horizontalScrollLock);
  s(io.verticalScrollLock);
  s(io.spriteDouble);
  s(io.spriteTile);
  s(io.frameInterrupts);
  s(io.displayEnable);
  s(io.mode);
  s(io.nameTableAddress);
  s(io.colorTableAddress);
  s(io.patternTableAddress);
  s(io.spriteAttributeTableAddress);
  s(io.spritePatternTableAddress);
  s(io.backdropColor);
  s(io.hscroll);
  s(io.vscroll);
  s(io.lineCounter);
}

auto VDP::Background::serialize(serializer& s) -> void {
  s(output.color);
  s(output.palette);
  s(output.priority);
}

auto VDP::Sprite::serialize(serializer& s) -> void {
  s(output.color);
  for(auto& object : objects) {
    s(object.x);
    s(object.y);
    s(object.pattern);
    s(object.color);
  }
  s(objectsValid);
}
