auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(ciram);
  s(cgram);
  s(oam);
  s(soam);
  s(var.data);
  s(scroll.data);
  s(scroll.fineX);
  s(scroll.latch);
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

  s(sprite);
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

auto PPU::SpriteEvaluation::serialize(serializer& s) -> void {
  s(io.spriteOverflow);
  s(io.oamAddress);
  s(io.oamData);
  s(io.oamTempCounterOverflow);
  s(io.oamTempCounter);
  s(io.oamTempCounterOverflow);
}
