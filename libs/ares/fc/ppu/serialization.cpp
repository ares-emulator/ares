auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(ciram);
  s(cgram);
  s(oam);
  s(soam);
  s(scroll.data);
  s(scroll.latch);
  s(scroll.fineX);
  s(scroll.transferDelay);
  s(var.data);
  s(var.latchData);
  s(var.blockingRead);
  s(io.busAddress);
  s(io.mdr);
  s(io.field);
  s(io.lx);
  s(io.ly);
  s(io.nmiHold);
  s(io.nmiFlag);
  s(io.vramIncrement);
  s(io.spriteAddress);
  s(io.bgAddress);
  s(io.spriteHeight);
  s(io.masterSelect);
  s(io.nmiEnable);
  s(io.grayscale);
  s(io.bgEdgeEnable);
  s(io.spriteEdgeEnable);
  s(io.bgEnable);
  s(io.spriteEnable);
  s(io.emphasis);
  s(io.spriteZeroHit);
  s(latch.nametable);
  s(latch.attribute);
  s(latch.tiledataLo);
  s(latch.tiledataHi);
  s(latch.oamId);
  for(auto& o : latch.oam) s(o);

  s(sprite.spriteOverflow);
  s(sprite.oamAddress);
  s(sprite.oamData);
  s(sprite.oamTempCounterOverflow);
  s(sprite.oamTempCounter);
  s(sprite.oamTempCounterOverflow);
}

auto PPU::OAM::serialize(serializer& s) -> void {
  s(id);
  s(y);
  s(tile);
  s(attr);
  s(x);
  s(tiledataLo);
  s(tiledataHi);
}
