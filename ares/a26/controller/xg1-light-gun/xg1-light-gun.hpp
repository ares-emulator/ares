struct XG1LightGun : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button trigger;
  Node::Video::Sprite sprite;

  XG1LightGun(Node::Port);
  ~XG1LightGun();

  auto poll() -> void override;
  auto read() -> n8 override;
  auto serialize(serializer&) -> void override;

private:
  auto lightDetected() const -> bool;
  auto updateCrosshair() -> void;

  i16 targetX = 80;
  i16 targetY = 114;

  static constexpr s32 TargetMargin = 16;
  static constexpr s32 HorizontalClocks = 228;
  static constexpr s32 HorizontalOffset = -23;
  static constexpr s32 VerticalOffset = 1;
  static constexpr s32 DetectionWidth = 15;
  static constexpr s32 CrosshairRadius = 8;
  static constexpr s32 VideoLeftMargin = 10;
};
