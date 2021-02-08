auto V9938::graphic4(n4& color, n8 hoffset, n8 voffset) -> void {
  n17 address = table.patternLayout & 0x18000;
  address += voffset << 7;
  address += hoffset >> 1;
  auto data = videoRAM.read(address);
  auto shift = !hoffset.bit(0) ? 4 : 0;
  color = n4(data >> shift);
}
