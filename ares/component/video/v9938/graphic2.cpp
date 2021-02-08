auto V9938::graphic2(n4& color, n8 hoffset, n8 voffset) -> void {
  n14 patternLayout;
  patternLayout.bit( 0, 4) = hoffset.bit(3,7);
  patternLayout.bit( 5, 9) = voffset.bit(3,7);
  patternLayout.bit(10,13) = table.patternLayout.bit(10,13);
  n8 pattern = videoRAM.read(patternLayout);

  n14 patternGenerator;
  patternGenerator.bit(0, 2) = voffset.bit(0,2);
  patternGenerator.bit(3,10) = pattern;
  if(voffset >=  64 && voffset <= 127) patternGenerator.bit(11) = table.patternGenerator.bit(11);
  if(voffset >= 128 && voffset <= 191) patternGenerator.bit(12) = table.patternGenerator.bit(12);

  n14 colorAddress = patternGenerator;
  patternGenerator.bit(13) = table.patternGenerator.bit(13);
  colorAddress.bit(13) = table.color.bit(13);

  n8 colorMask = table.color.bit(6,12) << 1 | 1;
  n8 output = videoRAM.read(colorAddress);
  if(videoRAM.read(patternGenerator).bit(~hoffset & 7)) output >>= 4;
  color = output.bit(0,3);
}
