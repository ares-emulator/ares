struct AtariMouse : RelativePointingDevice {
  AtariMouse(Node::Port);

protected:
  auto encode(u8 xPhase, u8 yPhase, bool left, bool down) const -> u8 override;
};
