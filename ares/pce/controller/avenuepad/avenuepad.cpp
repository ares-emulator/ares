AvenuePad::AvenuePad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Avenue Pad 6");

  up     = node->append<Node::Input::Button>("Up");
  down   = node->append<Node::Input::Button>("Down");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  three  = node->append<Node::Input::Button>("III");
  two    = node->append<Node::Input::Button>("II");
  one    = node->append<Node::Input::Button>("I");
  four   = node->append<Node::Input::Button>("IV");
  five   = node->append<Node::Input::Button>("V");
  six    = node->append<Node::Input::Button>("VI");
  select = node->append<Node::Input::Button>("Select");
  run    = node->append<Node::Input::Button>("Run");
}

auto AvenuePad::read() -> n4 {
  if(clr) return 0;

  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(three);
  platform->input(two);
  platform->input(one);
  platform->input(four);
  platform->input(five);
  platform->input(six);
  platform->input(select);
  platform->input(run);

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

  n4 data;

  if(active == 0) {
    if(sel) {
      data.bit(0) = !upLatch;
      data.bit(1) = !rightLatch;
      data.bit(2) = !downLatch;
      data.bit(3) = !leftLatch;
    } else {
      data.bit(0) = !one->value();
      data.bit(1) = !two->value();
      data.bit(2) = !select->value();
      data.bit(3) = !run->value();
    }
  }

  if(active == 1) {
    if(sel) {
      data.bit(0) = 0;
      data.bit(1) = 0;
      data.bit(2) = 0;
      data.bit(3) = 0;
    } else {
      data.bit(0) = !three->value();
      data.bit(1) = !four->value();
      data.bit(2) = !five->value();
      data.bit(3) = !six->value();
    }
  }

  return data;
}

auto AvenuePad::write(n2 data) -> void {
  //there should be a small delay for this to take effect ...
  if(!sel && data.bit(0)) active ^= 1;

  sel = data.bit(0);
  clr = data.bit(1);
}
