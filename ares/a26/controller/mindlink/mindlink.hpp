struct MindLink : Controller {
  Node::Input::Axis x;
  Node::Input::Button trigger;

  MindLink(Node::Port);

  auto poll() -> void override;
  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto controlWrite(n8 data) -> void override;
  auto serialize(serializer&) -> void override;

private:
  auto nextBit() -> void;

  u32 position = 0x2a00;
  u32 shift = 1;
  n5 lines = 0x1f;

  static constexpr s32 PositionScale = 10;
  static constexpr s32 MinimumPosition = 0x0b00;
  static constexpr s32 MaximumPosition = 0x6500;
  static constexpr u32 TriggerValue = 0x8000;
};
