struct Paddles : Controller {
  Node::Input::Axis axis[2];
  Node::Input::Button fire[2];

  Paddles(Node::Port);

  auto read() -> n8 override;
  auto readAnalogA() -> AnalogConnection override;
  auto readAnalogB() -> AnalogConnection override;
  auto readAxis(n1 index) -> AnalogConnection;

  static constexpr u32 MaximumResistance = 1'000'000;
};
