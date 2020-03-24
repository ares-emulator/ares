Gamepad::Gamepad(Node::Port parent, Node::Peripheral with) {
  node = Node::append<Node::Peripheral>(parent, with, "Gamepad");

  up    = Node::append<Node::Button>(node, with, "Up");
  down  = Node::append<Node::Button>(node, with, "Down");
  left  = Node::append<Node::Button>(node, with, "Left");
  right = Node::append<Node::Button>(node, with, "Right");
  l     = Node::append<Node::Button>(node, with, "L");
  r     = Node::append<Node::Button>(node, with, "R");
  one   = Node::append<Node::Button>(node, with, "1");
  two   = Node::append<Node::Button>(node, with, "2");
  three = Node::append<Node::Button>(node, with, "3");
  four  = Node::append<Node::Button>(node, with, "4");
  five  = Node::append<Node::Button>(node, with, "5");
  six   = Node::append<Node::Button>(node, with, "6");
  seven = Node::append<Node::Button>(node, with, "7");
  eight = Node::append<Node::Button>(node, with, "8");
  nine  = Node::append<Node::Button>(node, with, "9");
  star  = Node::append<Node::Button>(node, with, "*");
  zero  = Node::append<Node::Button>(node, with, "0");
  pound = Node::append<Node::Button>(node, with, "#");
}

auto Gamepad::read() -> uint8 {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(l);
  platform->input(r);
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
  platform->input(zero);
  platform->input(pound);

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
  if(select == 0) {
         if(one->value  ()) data.bit(0,3) = 0b1101;
    else if(two->value  ()) data.bit(0,3) = 0b0111;
    else if(three->value()) data.bit(0,3) = 0b1100;
    else if(four->value ()) data.bit(0,3) = 0b0010;
    else if(five->value ()) data.bit(0,3) = 0b0011;
    else if(six->value  ()) data.bit(0,3) = 0b1110;
    else if(seven->value()) data.bit(0,3) = 0b0101;
    else if(eight->value()) data.bit(0,3) = 0b0001;
    else if(nine->value ()) data.bit(0,3) = 0b1011;
    else if(star->value ()) data.bit(0,3) = 0b1001;
    else if(zero->value ()) data.bit(0,3) = 0b1010;
    else if(pound->value()) data.bit(0,3) = 0b0110;
    data.bit(6) = !r->value();
  } else {
    data.bit(0) = !upLatch;
    data.bit(1) = !rightLatch;
    data.bit(2) = !downLatch;
    data.bit(3) = !leftLatch;
    data.bit(6) = !l->value();
  }
  return data;
}

auto Gamepad::write(uint8 data) -> void {
  select = data.bit(0);
}
