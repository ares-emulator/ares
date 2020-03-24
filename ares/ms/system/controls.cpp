auto System::Controls::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Object>(parent, from, "Controls");
  from = Node::scan(parent = node, from);

  if(MasterSystem::Model::MasterSystem()) {
    pause = Node::append<Node::Button>(parent, from, "Pause");
    reset = Node::append<Node::Button>(parent, from, "Reset");
  }

  if(MasterSystem::Model::GameGear()) {
    up    = Node::append<Node::Button>(parent, from, "Up");
    down  = Node::append<Node::Button>(parent, from, "Down");
    left  = Node::append<Node::Button>(parent, from, "Left");
    right = Node::append<Node::Button>(parent, from, "Right");
    one   = Node::append<Node::Button>(parent, from, "1");
    two   = Node::append<Node::Button>(parent, from, "2");
    start = Node::append<Node::Button>(parent, from, "Start");
  }
}

auto System::Controls::poll() -> void {
  if(MasterSystem::Model::MasterSystem()) {
    auto paused = pause->value();
    platform->input(pause);
    platform->input(reset);

    if(!paused && pause->value()) cpu.setNMI(1);
  }

  if(MasterSystem::Model::GameGear()) {
    platform->input(up);
    platform->input(down);
    platform->input(left);
    platform->input(right);
    platform->input(one);
    platform->input(two);
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
}
