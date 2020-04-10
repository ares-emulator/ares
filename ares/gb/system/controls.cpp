auto System::Controls::load(Node::Object parent) -> void {
  if(GameBoy::Model::SuperGameBoy()) return;  //inputs provided by SNES gamepad(s)

  node = parent->append<Node::Object>("Controls");

  up     = node->append<Node::Button>("Up");
  down   = node->append<Node::Button>("Down");
  left   = node->append<Node::Button>("Left");
  right  = node->append<Node::Button>("Right");
  b      = node->append<Node::Button>("B");
  a      = node->append<Node::Button>("A");
  select = node->append<Node::Button>("Select");
  start  = node->append<Node::Button>("Start");
}

auto System::Controls::poll() -> void {
  if(GameBoy::Model::SuperGameBoy()) return;

  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(b);
  platform->input(a);
  platform->input(select);
  platform->input(start);

  if(!(up->value() & down->value())) {
    yHold = 0, upLatch = up->value(), downLatch = down->value();
  } else if(!yHold) {
    yHold = 1, swap(upLatch, downLatch);
  }

  if(!(left->value() & right->value())) {
    xHold = 0, leftLatch = left->value(), rightLatch = right->value();
  } else if(!xHold) {
    xHold = 1, swap(leftLatch, downLatch);
  }
}
