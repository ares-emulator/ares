auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  up    = node->append<Node::Input::Button>("Up");
  down  = node->append<Node::Input::Button>("Down");
  left  = node->append<Node::Input::Button>("Left");
  right = node->append<Node::Input::Button>("Right");
  fire  = node->append<Node::Input::Button>("Fire");
}

auto System::Controls::poll() -> void {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(fire);
}

//active low, and the three unused lines float high
auto System::Controls::read() -> n8 {
  n8 data;
  data.bit(0) = fire->value();
  data.bit(1) = up->value();
  data.bit(2) = down->value();
  data.bit(3) = right->value();
  data.bit(4) = left->value();
  return data ^ 0xff;
}
