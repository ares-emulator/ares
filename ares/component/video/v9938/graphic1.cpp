auto V9938::graphic1(uint4& color, uint8 hoffset, uint8 voffset) -> void {
  uint14 patternLayout;
  patternLayout.bit( 0, 4) = hoffset.bit(3,7);
  patternLayout.bit( 5, 9) = voffset.bit(3,7);
  patternLayout.bit(10,13) = table.patternLayout.bit(10,13);
  uint8 pattern = videoRAM.read(patternLayout);

  uint14 patternGenerator;
  patternGenerator.bit( 0, 2) = voffset.bit(0,2);
  patternGenerator.bit( 3,10) = pattern;
  patternGenerator.bit(11,13) = table.patternGenerator.bit(11,13);

  uint14 colorAddress;  //d5 = 0
  colorAddress.bit(0, 4) = pattern.bit(3,7);
  colorAddress.bit(6,13) = table.color.bit(6,13);

  uint8 output = videoRAM.read(colorAddress);
  if(videoRAM.read(patternGenerator).bit(~hoffset & 7)) output >>= 4;
  color = output.bit(0,3);
}
