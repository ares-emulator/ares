auto System::ArcadeControls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  p1up    = node->append<Node::Input::Button>("Player 1 Up");
  p1down  = node->append<Node::Input::Button>("Player 1 Down");
  p1left  = node->append<Node::Input::Button>("Player 1 Left");
  p1right = node->append<Node::Input::Button>("Player 1 Right");
  p1b1    = node->append<Node::Input::Button>("Player 1 Button 1");
  p1b2    = node->append<Node::Input::Button>("Player 1 Button 2");
  p1start = node->append<Node::Input::Button>("Player 1 Start");
  p1coin  = node->append<Node::Input::Button>("Player 1 Coin");
  service = node->append<Node::Input::Button>("Service");
  p2up    = node->append<Node::Input::Button>("Player 2 Up");
  p2down  = node->append<Node::Input::Button>("Player 2 Down");
  p2left  = node->append<Node::Input::Button>("Player 2 Left");
  p2right = node->append<Node::Input::Button>("Player 2 Right");
  p2b1    = node->append<Node::Input::Button>("Player 2 Button 1");
  p2b2    = node->append<Node::Input::Button>("Player 2 Button 2");
  p2start = node->append<Node::Input::Button>("Player 2 Start");
  p2coin  = node->append<Node::Input::Button>("Player 2 Coin");
}

auto System::ArcadeControls::poll() -> void {
  platform->input(p1up);
  platform->input(p1down);
  platform->input(p1left);
  platform->input(p1right);
  platform->input(p1b1);
  platform->input(p1b2);
  platform->input(p1start);
  platform->input(p2start);
  platform->input(p1coin);
  platform->input(p2coin);
  platform->input(service);
  platform->input(p2up);
  platform->input(p2down);
  platform->input(p2left);
  platform->input(p2right);
  platform->input(p2b1);
  platform->input(p2b2);
}
