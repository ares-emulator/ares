struct Joy2BPlus : Gamepad {
  Node::Input::Button buttonC;
  Node::Input::Button button3;

  Joy2BPlus(Node::Port);

  auto readAnalog(n1 index) -> AnalogConnection override;
};
