ControlPad::ControlPad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Control Pad");

  up    = node->append<Node::Input::Button>("Up");
  down  = node->append<Node::Input::Button>("Down");
  left  = node->append<Node::Input::Button>("Left");
  right = node->append<Node::Input::Button>("Right");
  a     = node->append<Node::Input::Button>("A");
  b     = node->append<Node::Input::Button>("B");
  c     = node->append<Node::Input::Button>("C");
  start = node->append<Node::Input::Button>("Start");
}

auto ControlPad::poll() -> void {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);
  platform->input(c);
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

auto ControlPad::readData() -> Data {
  n6 data;

  if(select == 0) {
    data.bit(0) = upLatch;
    data.bit(1) = downLatch;
    data.bit(2,3) = ~0;
    data.bit(4) = a->value();
    data.bit(5) = start->value();
  } else {
    data.bit(0) = upLatch;
    data.bit(1) = downLatch;
    data.bit(2) = leftLatch;
    data.bit(3) = rightLatch;
    data.bit(4) = b->value();
    data.bit(5) = c->value();
  }

  data = ~data;
  return {data, 0x3f};
}

auto ControlPad::writeData(n8 data) -> void {
  select = data.bit(6);
}
