auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  up     = node->append<Node::Input::Button>("Up");
  down   = node->append<Node::Input::Button>("Down");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  a      = node->append<Node::Input::Button>("A");
  b      = node->append<Node::Input::Button>("B");
  start  = node->append<Node::Input::Button>("Start");
  select = node->append<Node::Input::Button>("Select");
}

auto System::Controls::poll() -> void {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);
  platform->input(start);
  platform->input(select);
}

//all eight lines are active low
auto System::Controls::read() -> n8 {
  n8 data;
  data.bit(0) = up->value();
  data.bit(1) = down->value();
  data.bit(2) = left->value();
  data.bit(3) = right->value();
  data.bit(4) = a->value();
  data.bit(5) = b->value();
  data.bit(6) = start->value();
  data.bit(7) = select->value();
  return ~data;
}
