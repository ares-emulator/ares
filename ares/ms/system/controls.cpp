auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  if(MasterSystem::Model::MasterSystem()) {
    pause = node->append<Node::Input::Button>("Pause");
    reset = node->append<Node::Input::Button>("Reset");
  }

  if(MasterSystem::Model::GameGear()) {
    up    = node->append<Node::Input::Button>("Up");
    down  = node->append<Node::Input::Button>("Down");
    left  = node->append<Node::Input::Button>("Left");
    right = node->append<Node::Input::Button>("Right");
    one   = node->append<Node::Input::Button>("1");
    two   = node->append<Node::Input::Button>("2");
    start = node->append<Node::Input::Button>("Start");
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
