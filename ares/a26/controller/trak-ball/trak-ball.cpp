TrakBall::TrakBall(Node::Port parent, string name)
: RelativePointingDevice(parent, name, 2) {
}

auto TrakBall::encode(u8 xPhase, u8 yPhase, bool left, bool down) const -> u8 {
  static constexpr u8 horizontal[2][2] = {
    {0b00, 0b01}, {0b10, 0b11},
  };
  static constexpr u8 vertical[2][2] = {
    {0b0100, 0b0000}, {0b1100, 0b1000},
  };
  return horizontal[xPhase & 1][left] | vertical[yPhase & 1][down];
}
