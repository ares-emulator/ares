AtariMouse::AtariMouse(Node::Port parent)
: RelativePointingDevice(parent, "Atari Mouse", 4) {
}

auto AtariMouse::encode(u8 xPhase, u8 yPhase, bool, bool) const -> u8 {
  static constexpr u8 horizontal[4] = {0b0000, 0b0001, 0b0011, 0b0010};
  static constexpr u8 vertical[4] = {0b0000, 0b0100, 0b1100, 0b1000};
  return horizontal[xPhase & 3] | vertical[yPhase & 3];
}
