struct SegaGenesis : Gamepad {
  Node::Input::Button buttonC;

  SegaGenesis(Node::Port);

  auto readAnalog(n1 index) -> AnalogConnection override;
};
