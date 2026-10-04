SegaGenesis::SegaGenesis(Node::Port parent) : Gamepad(parent, "Sega Genesis") {
  buttonC = node->append<Node::Input::Button>("C");
}

auto SegaGenesis::readAnalog(n1 index) -> AnalogConnection {
  if(!index) return AnalogConnection::vcc();
  platform->input(buttonC);
  return buttonC->value() ? AnalogConnection::ground() : AnalogConnection::vcc();
}
