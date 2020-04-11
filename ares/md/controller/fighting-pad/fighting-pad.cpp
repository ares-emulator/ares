FightingPad::FightingPad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Fighting Pad");

  up    = node->append<Node::Button>("Up");
  down  = node->append<Node::Button>("Down");
  left  = node->append<Node::Button>("Left");
  right = node->append<Node::Button>("Right");
  a     = node->append<Node::Button>("A");
  b     = node->append<Node::Button>("B");
  c     = node->append<Node::Button>("C");
  x     = node->append<Node::Button>("X");
  y     = node->append<Node::Button>("Y");
  z     = node->append<Node::Button>("Z");
  mode  = node->append<Node::Button>("Mode");
  start = node->append<Node::Button>("Start");

  Thread::create(1'000'000, {&FightingPad::main, this});
}

auto FightingPad::main() -> void {
  if(timeout) {
    timeout--;
  } else {
    counter = 0;
  }
  Thread::step(1);
  Thread::synchronize(cpu);
}

auto FightingPad::readData() -> uint8 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(a);
  platform->input(b);
  platform->input(c);
  platform->input(x);
  platform->input(y);
  platform->input(z);
  platform->input(mode);
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

  uint6 data;

  if(select == 0) {
    if(counter == 0 || counter == 1 || counter == 4) {
      data.bit(0) = upLatch;
      data.bit(1) = downLatch;
      data.bit(2,3) = ~0;
    }

    if(counter == 2) {
      data.bit(0,3) = ~0;  //controller type detection
    }

    if(counter == 3) {
      data.bit(0,3) = 0;
    }

    data.bit(4) = a->value();
    data.bit(5) = start->value();
  } else {
    if(counter == 0 || counter == 1 || counter == 2 || counter == 4) {
      data.bit(0) = upLatch;
      data.bit(1) = downLatch;
      data.bit(2) = leftLatch;
      data.bit(3) = rightLatch;
      data.bit(4) = b->value();
      data.bit(5) = c->value();
    }

    if(counter == 3) {
      data.bit(0) = z->value();
      data.bit(1) = y->value();
      data.bit(2) = x->value();
      data.bit(3) = mode->value();
      data.bit(4,5) = 0;
    }
  }

  data = ~data;
  return latch << 7 | select << 6 | data;
}

auto FightingPad::writeData(uint8 data) -> void {
  if(!select && data.bit(6)) {  //0->1 transition
    if(++counter == 5) counter = 0;
  }

  select = data.bit(6);
  latch = data.bit(7);
  timeout = 1600;  //~1.6ms
}
