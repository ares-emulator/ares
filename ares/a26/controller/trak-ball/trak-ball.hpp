struct TrakBall : RelativePointingDevice {
  TrakBall(Node::Port, string name);

protected:
  auto encode(u8 xPhase, u8 yPhase, bool left, bool down) const -> u8 override;
};
