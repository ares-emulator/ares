NTTDataKeypad::NTTDataKeypad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("NTT Data Keypad");

  up    = node->append<Node::Input::Button>("Up");
  down  = node->append<Node::Input::Button>("Down");
  left  = node->append<Node::Input::Button>("Left");
  right = node->append<Node::Input::Button>("Right");
  b     = node->append<Node::Input::Button>("B");
  a     = node->append<Node::Input::Button>("A");
  y     = node->append<Node::Input::Button>("Y");
  x     = node->append<Node::Input::Button>("X");
  l     = node->append<Node::Input::Button>("L");
  r     = node->append<Node::Input::Button>("R");
  back  = node->append<Node::Input::Button>("Back");
  next  = node->append<Node::Input::Button>("Next");
  one   = node->append<Node::Input::Button>("1");
  two   = node->append<Node::Input::Button>("2");
  three = node->append<Node::Input::Button>("3");
  four  = node->append<Node::Input::Button>("4");
  five  = node->append<Node::Input::Button>("5");
  six   = node->append<Node::Input::Button>("6");
  seven = node->append<Node::Input::Button>("7");
  eight = node->append<Node::Input::Button>("8");
  nine  = node->append<Node::Input::Button>("9");
  zero  = node->append<Node::Input::Button>("0");
  star  = node->append<Node::Input::Button>("*");
  clear = node->append<Node::Input::Button>("C");
  pound = node->append<Node::Input::Button>("#");
  point = node->append<Node::Input::Button>(".");
  end   = node->append<Node::Input::Button>("End");
}

auto NTTDataKeypad::data() -> n2 {
  if(latched == 1) {
    platform->input(b);
    return b->value();
  }

  switch(counter++) {
  case  0: return b->value();
  case  1: return y->value();
  case  2: return back->value();
  case  3: return next->value();
  case  4: return upLatch;
  case  5: return downLatch;
  case  6: return leftLatch;
  case  7: return rightLatch;
  case  8: return a->value();
  case  9: return x->value();
  case 10: return l->value();
  case 11: return r->value();

  case 12: return 1;  //4-bit device signature
  case 13: return 0;
  case 14: return 1;
  case 15: return 1;

  case 16: return zero->value();
  case 17: return one->value();
  case 18: return two->value();
  case 19: return three->value();
  case 20: return four->value();
  case 21: return five->value();
  case 22: return six->value();
  case 23: return seven->value();
  case 24: return eight->value();
  case 25: return nine->value();
  case 26: return star->value();
  case 27: return pound->value();
  case 28: return point->value();
  case 29: return clear->value();
  case 30: return 0;  //unverified (likely correct)
  case 31: return end->value();
  }

  counter = 32;
  return 1;  //unverified (likely correct)
}

auto NTTDataKeypad::latch(n1 data) -> void {
  if(latched == data) return;
  latched = data;
  counter = 0;

  if(latched == 0) {
    platform->input(b);
    platform->input(y);
    platform->input(back);
    platform->input(next);
    platform->input(up);
    platform->input(down);
    platform->input(left);
    platform->input(right);
    platform->input(a);
    platform->input(x);
    platform->input(l);
    platform->input(r);
    platform->input(zero);
    platform->input(one);
    platform->input(two);
    platform->input(three);
    platform->input(four);
    platform->input(five);
    platform->input(six);
    platform->input(seven);
    platform->input(eight);
    platform->input(nine);
    platform->input(star);
    platform->input(pound);
    platform->input(point);
    platform->input(clear);
    platform->input(end);

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
