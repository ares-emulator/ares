Paddles::Paddles(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Paddles");

  axis[0] = node->append<Node::Input::Axis>("Paddle 1");
  axis[1] = node->append<Node::Input::Axis>("Paddle 2");
  fire[0] = node->append<Node::Input::Button>("Paddle 1 Fire");
  fire[1] = node->append<Node::Input::Button>("Paddle 2 Fire");
}

auto Paddles::read() -> n8 {
  platform->input(fire[0]);
  platform->input(fire[1]);

  n8 data = 0xff;
  data.bit(3) = !fire[0]->value();
  data.bit(2) = !fire[1]->value();
  return data;
}

auto Paddles::readAnalogA() -> AnalogConnection {
  return readAxis(0);
}

auto Paddles::readAnalogB() -> AnalogConnection {
  return readAxis(1);
}

auto Paddles::readAxis(n1 index) -> AnalogConnection {
  platform->input(axis[index]);
  auto value = std::clamp<s64>(axis[index]->value(), -32768, 32767);
  auto resistance = (u64)(32767 - value) * MaximumResistance / 65535;
  return AnalogConnection::vcc(resistance);
}
