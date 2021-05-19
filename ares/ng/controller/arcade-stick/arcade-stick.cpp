ArcadeStick::ArcadeStick(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Arcade Stick");

  up     = node->append<Node::Input::Button>("Up");
  down   = node->append<Node::Input::Button>("Down");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  a      = node->append<Node::Input::Button>("A");
  b      = node->append<Node::Input::Button>("B");
  c      = node->append<Node::Input::Button>("C");
  d      = node->append<Node::Input::Button>("D");
  select = node->append<Node::Input::Button>("Select");
  start  = node->append<Node::Input::Button>("Start");
}

auto ArcadeStick::readButtons() -> n8 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);
  platform->input(c);
  platform->input(d);

  if(!(up->value() && down->value())) {
    yHold = 0, upLatch = up->value(), downLatch = down->value();
  } else if(!yHold) {
    yHold = 1, swap(upLatch, downLatch);
  }

  if(!(left->value() && right->value())) {
    xHold = 0, leftLatch = left->value(), rightLatch = right->value();
  } else if(!xHold) {
    xHold = 1, swap(leftLatch, rightLatch);
  }

  n8 data;
  data.bit(0) = upLatch;
  data.bit(1) = downLatch;
  data.bit(2) = leftLatch;
  data.bit(3) = rightLatch;
  data.bit(4) = a->value();
  data.bit(5) = b->value();
  data.bit(6) = c->value();
  data.bit(7) = d->value();
  return ~data;
}

auto ArcadeStick::readControls() -> n2 {
  platform->input(select);
  platform->input(start);

  n2 data;
  data.bit(0) = start->value();
  data.bit(1) = select->value();
  return ~data;
}
