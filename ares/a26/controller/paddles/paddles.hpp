struct Paddles : Controller {
  Node::Input::Axis axis[2];
  Node::Input::Button fire[2];

  Paddles(Node::Port);

  auto read() -> n8 override;
  auto readAnalog(n1 index) -> AnalogConnection override;

  static constexpr u32 MaximumResistance = 1'000'000;
};
