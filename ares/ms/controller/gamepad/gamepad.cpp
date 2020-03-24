Gamepad::Gamepad(Node::Port parent, Node::Peripheral with) {
  node = Node::Peripheral::create("Gamepad");
  node->load(with);
  up = Node::append<Node::Button>(node, with, "Up");
  down = Node::append<Node::Button>(node, with, "Down");
  left = Node::append<Node::Button>(node, with, "Left");
  right = Node::append<Node::Button>(node, with, "Right");
  one = Node::append<Node::Button>(node, with, "1");
  two = Node::append<Node::Button>(node, with, "2");
  parent->prepend(node);
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
