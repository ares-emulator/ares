auto V9938::graphic4(uint4& color, uint8 hoffset, uint8 voffset) -> void {
  uint17 address = table.patternLayout & 0x18000;
  address += voffset << 7;
  address += hoffset >> 1;
  auto data = videoRAM.read(address);
  auto shift = !hoffset.bit(0) ? 4 : 0;
  color = uint4(data >> shift);
}
