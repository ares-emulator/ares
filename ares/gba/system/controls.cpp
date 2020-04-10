auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  up     = node->append<Node::Button>("Up");
  down   = node->append<Node::Button>("Down");
  left   = node->append<Node::Button>("Left");
  right  = node->append<Node::Button>("Right");
  b      = node->append<Node::Button>("B");
  a      = node->append<Node::Button>("A");
  l      = node->append<Node::Button>("L");
  r      = node->append<Node::Button>("R");
  select = node->append<Node::Button>("Select");
  start  = node->append<Node::Button>("Start");

  if(GameBoyAdvance::Model::GameBoyPlayer()) {
    rumbler = parent->append<Node::Rumble>("Rumble");
  }
}

auto System::Controls::poll() -> void {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(b);
  platform->input(a);
  platform->input(l);
  platform->input(r);
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
    xHold = 1, swap(leftLatch, rightLatch);
  }
}

auto System::Controls::rumble(bool enable) -> void {
  if(GameBoyAdvance::Model::GameBoyPlayer()) {
    rumbler->setEnable(enable);
    platform->input(rumbler);
  }
}
