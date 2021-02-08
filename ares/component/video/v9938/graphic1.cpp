auto V9938::graphic1(n4& color, n8 hoffset, n8 voffset) -> void {
  n14 patternLayout;
  patternLayout.bit( 0, 4) = hoffset.bit(3,7);
  patternLayout.bit( 5, 9) = voffset.bit(3,7);
  patternLayout.bit(10,13) = table.patternLayout.bit(10,13);
  n8 pattern = videoRAM.read(patternLayout);

  n14 patternGenerator;
  patternGenerator.bit( 0, 2) = voffset.bit(0,2);
  patternGenerator.bit( 3,10) = pattern;
  patternGenerator.bit(11,13) = table.patternGenerator.bit(11,13);

  n14 colorAddress;  //d5 = 0
  colorAddress.bit(0, 4) = pattern.bit(3,7);
  colorAddress.bit(6,13) = table.color.bit(6,13);

  n8 output = videoRAM.read(colorAddress);
  if(videoRAM.read(patternGenerator).bit(~hoffset & 7)) output >>= 4;
  color = output.bit(0,3);
}
