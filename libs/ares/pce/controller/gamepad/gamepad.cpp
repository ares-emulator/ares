Gamepad::Gamepad(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Gamepad");

  up     = node->append<Node::Input::Button>("Up");
  down   = node->append<Node::Input::Button>("Down");
  left   = node->append<Node::Input::Button>("Left");
  right  = node->append<Node::Input::Button>("Right");
  two    = node->append<Node::Input::Button>("II");
  one    = node->append<Node::Input::Button>("I");
  select = node->append<Node::Input::Button>("Select");
  run    = node->append<Node::Input::Button>("Run");
}

auto Gamepad::read() -> n4 {
  if(clr) return 0;

  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(two);
  platform->input(one);
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

  return data;
}

auto Gamepad::write(n2 data) -> void {
  //there should be a small delay for this to take effect ...
  sel = data.bit(0);
  clr = data.bit(1);
}
