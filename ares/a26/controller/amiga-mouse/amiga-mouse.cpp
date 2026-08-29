AmigaMouse::AmigaMouse(Node::Port parent)
: RelativePointingDevice(parent, "Amiga Mouse", 4) {
}

auto AmigaMouse::encode(u8 xPhase, u8 yPhase, bool, bool) const -> u8 {
  static constexpr u8 horizontal[4] = {0b0000, 0b1000, 0b1010, 0b0010};
  static constexpr u8 vertical[4] = {0b0000, 0b0100, 0b0101, 0b0001};
  return horizontal[xPhase & 3] | vertical[yPhase & 3];
}
