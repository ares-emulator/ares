//graphic mode 3 is identical to graphic mode 2:
//the difference is only that mode 2 uses sprite mode 1, and mode 3 uses sprite mode 2
auto V9938::graphic3(uint4& color, uint8 hoffset, uint8 voffset) -> void {
  return graphic2(color, hoffset, voffset);
}
