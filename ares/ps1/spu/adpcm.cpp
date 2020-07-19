auto SPU::Voice::readBlock() -> void {
  uint19 address = adpcm.currentAddress;

  uint16 header = self.readRAM(address);
  block.shift      = header.bit(0,3) > 12 ? 9 : header.bit(0,3);
  block.filter     = header.bit(4,6) >  4 ? 4 : header.bit(4,6);
  block.loopEnd    = header.bit( 8);
  block.loopRepeat = header.bit( 9);
  block.loopStart  = header.bit(10);
  address += 2;

  for(uint word : range(7)) {
    uint16 data = self.readRAM(address);
    block.brr[word << 2 | 0] = data >>  0 & 15;
    block.brr[word << 2 | 1] = data >>  4 & 15;
    block.brr[word << 2 | 2] = data >>  8 & 15;
    block.brr[word << 2 | 3] = data >> 12 & 15;
    address += 2;
  }
}

auto SPU::Voice::decodeBlock() -> void {
  static const i32 positive[5] = {0, 60, 115,  98, 122};
  static const i32 negative[5] = {0,  0, -52, -55, -60};

  //cache samples for later interpolation
  adpcm.previousSamples[0] = adpcm.currentSamples[25];
  adpcm.previousSamples[1] = adpcm.currentSamples[26];
  adpcm.previousSamples[2] = adpcm.currentSamples[27];

  i16 lastSamples[2] = {adpcm.lastSamples[0], adpcm.lastSamples[1]};
  for(uint nibble : range(28)) {
    i32 sample = i16(block.brr[nibble] << 12) >> block.shift;
    sample += lastSamples[0] * positive[block.filter] >> 6;
    sample += lastSamples[1] * negative[block.filter] >> 6;
    lastSamples[1] = lastSamples[0];
    lastSamples[0] = sclamp<16>(sample);
    adpcm.currentSamples[nibble] = lastSamples[0];
  }
  adpcm.lastSamples[0] = lastSamples[0];
  adpcm.lastSamples[1] = lastSamples[1];
}
