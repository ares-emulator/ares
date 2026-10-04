struct RelativePointingDevice : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button fire;

  RelativePointingDevice(Node::Port, string name, u32 scaleNumerator);

  auto poll() -> void override;
  auto read() -> n8 override;
  auto serialize(serializer&) -> void override;

protected:
  virtual auto encode(u8 xPhase, u8 yPhase, bool left, bool down) const -> u8 = 0;

private:
  struct AxisState {
    u8 phase = 0;
    s8 direction = 0;
    u32 remaining = 0;
    u64 next = 0;
    u64 interval = 1;
    s32 remainder = 0;
  } horizontal, vertical;

  auto finish(AxisState&) -> void;
  auto queue(AxisState&, s64 delta) -> void;
  auto advance(AxisState&, u64 timestamp) -> void;
  auto windowDuration() const -> u64;

  const u32 scaleNumerator;
  static constexpr u32 ScaleDenominator = 5;
  static constexpr u32 MaximumTransitions = 32767;
};
