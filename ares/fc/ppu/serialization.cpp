auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(ciram);
  s(cgram);
  s(oam);
  s(io.mdr);
  s(io.field);
  s(io.lx);
  s(io.ly);
  s(io.busData);
  s(io.v.data);
  s(io.t.data);
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
  s(io.spriteOverflow);
  s(io.spriteZeroHit);
  s(io.oamAddress);
  s(latch.nametable);
  s(latch.attribute);
  s(latch.tiledataLo);
  s(latch.tiledataHi);
  s(latch.oamIterator);
  s(latch.oamCounter);
  for(auto& o : latch.oam) s(o);
  for(auto& o : latch.soam) s(o);
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
