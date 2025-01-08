auto Aleck64::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  service = node->append<Node::Input::Button>("Service");

  p1x     = node->append<Node::Input::Axis>  ("Player 1 X-Axis");
  p1y     = node->append<Node::Input::Axis>  ("Player 1 Y-Axis");
  p1up    = node->append<Node::Input::Button>("Player 1 Up");
  p1down  = node->append<Node::Input::Button>("Player 1 Down");
  p1left  = node->append<Node::Input::Button>("Player 1 Left");
  p1right = node->append<Node::Input::Button>("Player 1 Right");
  p1start = node->append<Node::Input::Button>("Player 1 Start");
  p1coin  = node->append<Node::Input::Button>("Player 1 Coin");

  for(auto n : range(9)) {
    string name = {"Player 1 Button ", 1 + n};
    p1[n] = node->append<Node::Input::Button>(name);
  }

  p2x     = node->append<Node::Input::Axis>  ("Player 2 X-Axis");
  p2y     = node->append<Node::Input::Axis>  ("Player 2 Y-Axis");
  p2up    = node->append<Node::Input::Button>("Player 2 Up");
  p2down  = node->append<Node::Input::Button>("Player 2 Down");
  p2left  = node->append<Node::Input::Button>("Player 2 Left");
  p2right = node->append<Node::Input::Button>("Player 2 Right");
  p2start = node->append<Node::Input::Button>("Player 2 Start");
  p2coin  = node->append<Node::Input::Button>("Player 2 Coin");

  for(auto n : range(9)) {
    string name = {"Player 2 Button ", 1 + n};
    p2[n] = node->append<Node::Input::Button>(name);
  }
}

auto Aleck64::Controls::poll() -> void {
  platform->input(service);

  platform->input(p1x);
  platform->input(p1y);
  platform->input(p1up);
  platform->input(p1down);
  platform->input(p1left);
  platform->input(p1right);
  platform->input(p1start);
  platform->input(p1coin);

  for(auto n : range(9)) {
    platform->input(p1[n]);
  }

  platform->input(p2x);
  platform->input(p2y);
  platform->input(p2up);
  platform->input(p2down);
  platform->input(p2left);
  platform->input(p2right);
  platform->input(p2start);
  platform->input(p2coin);

  for(auto n : range(9)) {
    platform->input(p2[n]);
  }
}

auto Aleck64::Controls::controllerButton(int playerIndex, string button) -> bool {
  if(auto input = self.gameConfig->controllerButton(playerIndex, button)) {
    return input;
  }
  return 0;
}

auto Aleck64::Controls::controllerAxis(int playerIndex, string axis) -> s64 {
  if(auto input = self.gameConfig->controllerAxis(playerIndex, axis)) {
    return input;
  }

  return 0;
}