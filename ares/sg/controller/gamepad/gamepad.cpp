Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  up    = node->append<Node::Button>("Up");
  down  = node->append<Node::Button>("Down");
  left  = node->append<Node::Button>("Left");
  right = node->append<Node::Button>("Right");
  one   = node->append<Node::Button>("1");
  two   = node->append<Node::Button>("2");
}

auto Gamepad::read() -> uint8 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(one);
  platform->input(two);

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

  uint8 data = 0xff;
  data.bit(0) = !upLatch;
  data.bit(1) = !downLatch;
  data.bit(2) = !leftLatch;
  data.bit(3) = !rightLatch;
  data.bit(4) = !one->value();
  data.bit(5) = !two->value();
  return data;
}
