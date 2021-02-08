Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  up     = node->append<Node::Input::Button>("Up");
  down   = node->append<Node::Input::Button>("Down");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  b      = node->append<Node::Input::Button>("B");
  a      = node->append<Node::Input::Button>("A");
  y      = node->append<Node::Input::Button>("Y");
  x      = node->append<Node::Input::Button>("X");
  l      = node->append<Node::Input::Button>("L");
  r      = node->append<Node::Input::Button>("R");
  select = node->append<Node::Input::Button>("Select");
  start  = node->append<Node::Input::Button>("Start");
}

auto Gamepad::data() -> n2 {
  if(latched == 1) {
    platform->input(b);
    return b->value();
  }

  //note: D-pad physically prevents up+down and left+right from being pressed at the same time
  switch(counter++) {
  case  0: return b->value();
  case  1: return y->value();
  case  2: return select->value();
  case  3: return start->value();
  case  4: return upLatch;
  case  5: return downLatch;
  case  6: return leftLatch;
  case  7: return rightLatch;
  case  8: return a->value();
  case  9: return x->value();
  case 10: return l->value();
  case 11: return r->value();

  case 12: return 0;  //4-bit device signature
  case 13: return 0;
  case 14: return 0;
  case 15: return 0;
  }

  counter = 16;
  return 1;
}

auto Gamepad::latch(n1 data) -> void {
  if(latched == data) return;
  latched = data;
  counter = 0;

  if(latched == 0) {
    platform->input(b);
    platform->input(y);
    platform->input(select);
    platform->input(start);
    platform->input(up);
    platform->input(down);
    platform->input(left);
    platform->input(right);
    platform->input(a);
    platform->input(x);
    platform->input(l);
    platform->input(r);

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
}
