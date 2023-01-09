Kempston::Kempston(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Kempston");

  up     = node->append<Node::Input::Button>("Up");
  down   = node->append<Node::Input::Button>("Down");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  fire   = node->append<Node::Input::Button>("Fire");
}

auto Kempston::in(n16 address) -> n8
{
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(fire);

  n8 value = 0;
  value.bit(0) = right->value();
  value.bit(1) = left->value();
  value.bit(2) = down->value();
  value.bit(3) = up->value();
  value.bit(4) = fire->value();

  return value;
}
