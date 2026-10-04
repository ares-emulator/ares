struct BoosterGrip : Gamepad {
  Node::Input::Button booster;
  Node::Input::Button trigger;

  BoosterGrip(Node::Port);

  auto readAnalog(n1 index) -> AnalogConnection override;
};
