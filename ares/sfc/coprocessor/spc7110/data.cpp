auto SPC7110::dataromRead(n24 address) -> n8 {
  n24 size = 1 << (r4834 & 3);  //size in MB
  n24 mask = 0x100000 * size - 1;
  n24 offset = address & mask;
  if((r4834 & 3) != 3 && (address & 0x400000)) return 0x00;
  return drom.read(Bus::mirror(offset, drom.size()));
}

auto SPC7110::dataOffset() -> n24 { return r4811 | r4812 << 8 | r4813 << 16; }
auto SPC7110::dataAdjust() -> n16 { return r4814 | r4815 << 8; }
auto SPC7110::dataStride() -> n16 { return r4816 | r4817 << 8; }
auto SPC7110::setDataOffset(n24 address) -> void { r4811 = address; r4812 = address >> 8; r4813 = address >> 16; }
auto SPC7110::setDataAdjust(n16 address) -> void { r4814 = address; r4815 = address >> 8; }

auto SPC7110::dataPortRead() -> void {
  n24 offset = dataOffset();
  n16 adjust = r4818 & 2 ? dataAdjust() : (n16)0;
  if(r4818 & 8) adjust = (i16)adjust;
  r4810 = dataromRead(offset + adjust);
}

auto SPC7110::dataPortIncrement4810() -> void {
  n24 offset = dataOffset();
  n16 stride = r4818 & 1 ? dataStride() : (n16)1;
  n16 adjust = dataAdjust();
  if(r4818 & 4) stride = (i16)stride;
  if(r4818 & 8) adjust = (i16)adjust;
  if((r4818 & 16) == 0) setDataOffset(offset + stride);
  if((r4818 & 16) != 0) setDataAdjust(adjust + stride);
  dataPortRead();
}

auto SPC7110::dataPortIncrement4814() -> void {
  if(r4818 >> 5 != 1) return;
  n24 offset = dataOffset();
  n16 adjust = dataAdjust();
  if(r4818 & 8) adjust = (i16)adjust;
  setDataOffset(offset + adjust);
  dataPortRead();
}

auto SPC7110::dataPortIncrement4815() -> void {
  if(r4818 >> 5 != 2) return;
  n24 offset = dataOffset();
  n16 adjust = dataAdjust();
  if(r4818 & 8) adjust = (i16)adjust;
  setDataOffset(offset + adjust);
  dataPortRead();
}

auto SPC7110::dataPortIncrement481a() -> void {
  if(r4818 >> 5 != 3) return;
  n24 offset = dataOffset();
  n16 adjust = dataAdjust();
  if(r4818 & 8) adjust = (i16)adjust;
  setDataOffset(offset + adjust);
  dataPortRead();
}
