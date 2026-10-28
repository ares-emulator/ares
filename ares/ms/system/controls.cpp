auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  if(Device::MasterSystem() && !MasterSystem::Model::SystemE()) {
    if(MasterSystem::Region::NTSCJ() == 1) rapid = node->append<Node::Input::Button>("Rapid");
    if(MasterSystem::Region::NTSCJ() == 0) reset = node->append<Node::Input::Button>("Reset");
    pause = node->append<Node::Input::Button>("Pause");
  }

  if(Device::GameGear()) {
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
  if(Device::MasterSystem() && !MasterSystem::Model::SystemE()) {
    if(MasterSystem::Region::NTSCJ() == 1) platform->input(rapid);
    if(MasterSystem::Region::NTSCJ() == 0) platform->input(reset);
    auto paused = pause->value();
    platform->input(pause);
    if(!paused && pause->value()) cpu.setNMI(1);
  }

  if(Device::GameGear()) {
    auto paused = start->value();
    platform->input(up);
    platform->input(down);
    platform->input(left);
    platform->input(right);
    platform->input(one);
    platform->input(two);
    platform->input(start);

    if(!paused && start->value()) {
      if(Mode::MasterSystem()) cpu.setNMI(1);
    }

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

auto System::ArcadeControls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");
  service = node->append<Node::Input::Button>("Service");
  for(u32 player : range(2)) {
    auto prefix = string{"Player ", player + 1, " "};
    coin[player] = node->append<Node::Input::Button>(string{prefix, "Coin"});
    start[player] = node->append<Node::Input::Button>(string{prefix, "Start"});
    up[player] = node->append<Node::Input::Button>(string{prefix, "Up"});
    down[player] = node->append<Node::Input::Button>(string{prefix, "Down"});
    left[player] = node->append<Node::Input::Button>(string{prefix, "Left"});
    right[player] = node->append<Node::Input::Button>(string{prefix, "Right"});
    button1[player] = node->append<Node::Input::Button>(string{prefix, "Button 1"});
    button2[player] = node->append<Node::Input::Button>(string{prefix, "Button 2"});
  }
  analogX = node->append<Node::Input::Axis>("Player 1 X-Axis");
  analogY = node->append<Node::Input::Axis>("Player 1 Y-Axis");
  accelerator = node->append<Node::Input::Axis>("Player 1 Accelerator");
}

auto System::ArcadeControls::poll() -> void {
  platform->input(service);
  for(u32 player : range(2)) {
    platform->input(coin[player]);
    platform->input(start[player]);
    platform->input(up[player]);
    platform->input(down[player]);
    platform->input(left[player]);
    platform->input(right[player]);
    platform->input(button1[player]);
    platform->input(button2[player]);
  }
  platform->input(analogX);
  platform->input(analogY);
  platform->input(accelerator);
  if(ppi.game == "ridleofp" || ppi.game == "megrescu") {
    ppi.x += (i32)analogX->value() >> 8;
    ppi.y += (i32)analogY->value() >> 8;
  }
}

auto System::ArcadeControls::coinPort() const -> n8 {
  n8 data = 0xff;
  data.bit(0) = !coin[0]->value();
  data.bit(1) = !coin[1]->value();
  data.bit(3) = !service->value();
  if(cartridge.pak && cartridge.pak->attribute("name") == "hangonjr") data.bit(4) = !start[0]->value();
  else data.bit(6) = !start[0]->value();
  data.bit(7) = !start[1]->value();
  return data;
}

auto System::ArcadeControls::playerPort(u32 player) const -> n8 {
  n8 data = 0xff;
  data.bit(0) = !up[player]->value();
  data.bit(1) = !down[player]->value();
  data.bit(2) = !left[player]->value();
  data.bit(3) = !right[player]->value();
  data.bit(4) = !button1[player]->value();
  data.bit(5) = !button2[player]->value();
  return data;
}
