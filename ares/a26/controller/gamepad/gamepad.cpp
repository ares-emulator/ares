Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  up    = node->append<Node::Input::Button>("Up");
  down  = node->append<Node::Input::Button>("Down");
  left  = node->append<Node::Input::Button>("Left");
  right = node->append<Node::Input::Button>("Right");
  fire  = node->append<Node::Input::Button>("Fire");
}

auto Gamepad::read() -> n8 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(fire);

  n8 data = 0xff;
  data.bit(0) = !up->value();
  data.bit(1) = !down->value();
  data.bit(2) = !left->value();
  data.bit(3) = !right->value();
  data.bit(4) = !fire->value();
  return data;
}
