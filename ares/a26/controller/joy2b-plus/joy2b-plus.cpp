Joy2BPlus::Joy2BPlus(Node::Port parent) : Gamepad(parent, "Joy 2B+") {
  buttonC = node->append<Node::Input::Button>("C");
  button3 = node->append<Node::Input::Button>("Button 3");
}

auto Joy2BPlus::readAnalog(n1 index) -> AnalogConnection {
  auto& button = index ? buttonC : button3;
  platform->input(button);
  return button->value() ? AnalogConnection::ground(330) : AnalogConnection::vcc();
}
