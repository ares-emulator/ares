BoosterGrip::BoosterGrip(Node::Port parent) : Gamepad(parent, "Booster Grip") {
  booster = node->append<Node::Input::Button>("Booster");
  trigger = node->append<Node::Input::Button>("Trigger");
}

auto BoosterGrip::readAnalog(n1 index) -> AnalogConnection {
  auto& button = index ? booster : trigger;
  platform->input(button);
  return button->value() ? AnalogConnection::vcc() : AnalogConnection::disconnected();
}
