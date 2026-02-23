auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  up       = node->append<Node::Input::Button>("Up");
  down     = node->append<Node::Input::Button>("Down");
  left     = node->append<Node::Input::Button>("Left");
  right    = node->append<Node::Input::Button>("Right");
  a        = node->append<Node::Input::Button>("A");
  b        = node->append<Node::Input::Button>("B");
  option   = node->append<Node::Input::Button>("Option");
  debugger = node->append<Node::Input::Button>("Debugger");
  power    = node->append<Node::Input::Button>("Power");
}

auto System::Controls::poll() -> void {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);
  platform->input(option);
  platform->input(debugger);

  if(!(up->value() & down->value())) {
    yHold = 0, upLatch = up->value(), downLatch = down->value();
  } else if(!yHold) {
    yHold = 1, swap(upLatch, downLatch);
  }

  if(!(left->value() & right->value())) {
    xHold = 0, leftLatch = left->value(), rightLatch = right->value();
  } else if(!xHold) {
    xHold = 1, swap(leftLatch, rightLatch);
  }
}
