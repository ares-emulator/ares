Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  up    = node->append<Node::Button>("Up");
  down  = node->append<Node::Button>("Down");
  left  = node->append<Node::Button>("Left");
  right = node->append<Node::Button>("Right");
  a     = node->append<Node::Button>("A");
  b     = node->append<Node::Button>("B");
}

auto Gamepad::read() -> uint6 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);

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

  uint6 data;
  data.bit(0) = !upLatch;
  data.bit(1) = !downLatch;
  data.bit(2) = !leftLatch;
  data.bit(3) = !rightLatch;
  data.bit(4) = !a->value();
  data.bit(5) = !b->value();
  return data;
}
