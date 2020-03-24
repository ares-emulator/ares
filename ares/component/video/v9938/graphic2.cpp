auto V9938::graphic2(uint4& color, uint8 hoffset, uint8 voffset) -> void {
  uint14 patternLayout;
  patternLayout.bit( 0, 4) = hoffset.bit(3,7);
  patternLayout.bit( 5, 9) = voffset.bit(3,7);
  patternLayout.bit(10,13) = table.patternLayout.bit(10,13);
  uint8 pattern = videoRAM.read(patternLayout);

  uint14 patternGenerator;
  patternGenerator.bit(0, 2) = voffset.bit(0,2);
  patternGenerator.bit(3,10) = pattern;
  if(voffset >=  64 && voffset <= 127) patternGenerator.bit(11) = table.patternGenerator.bit(11);
  if(voffset >= 128 && voffset <= 191) patternGenerator.bit(12) = table.patternGenerator.bit(12);

  uint14 colorAddress = patternGenerator;
  patternGenerator.bit(13) = table.patternGenerator.bit(13);
  colorAddress.bit(13) = table.color.bit(13);

  uint8 colorMask = table.color.bit(6,12) << 1 | 1;
  uint8 output = videoRAM.read(colorAddress);
  if(videoRAM.read(patternGenerator).bit(~hoffset & 7)) output >>= 4;
  color = output.bit(0,3);
}
