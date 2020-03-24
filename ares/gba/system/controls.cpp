auto System::Controls::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Object>(parent, from, "Controls");
  from = Node::scan(parent = node, from);

  up     = Node::append<Node::Button>(parent, from, "Up");
  down   = Node::append<Node::Button>(parent, from, "Down");
  left   = Node::append<Node::Button>(parent, from, "Left");
  right  = Node::append<Node::Button>(parent, from, "Right");
  b      = Node::append<Node::Button>(parent, from, "B");
  a      = Node::append<Node::Button>(parent, from, "A");
  l      = Node::append<Node::Button>(parent, from, "L");
  r      = Node::append<Node::Button>(parent, from, "R");
  select = Node::append<Node::Button>(parent, from, "Select");
  start  = Node::append<Node::Button>(parent, from, "Start");

  if(GameBoyAdvance::Model::GameBoyPlayer()) {
    rumbler = Node::append<Node::Rumble>(parent, from, "Rumble");
  }
}

auto System::Controls::poll() -> void {
  platform->input(up);
  platform->input(down);
  platform->input(left);
  platform->input(right);
  platform->input(b);
  platform->input(a);
  platform->input(l);
  platform->input(r);
  platform->input(select);
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

auto System::Controls::rumble(bool enable) -> void {
  if(GameBoyAdvance::Model::GameBoyPlayer()) {
    rumbler->setEnable(enable);
    platform->input(rumbler);
  }
}
