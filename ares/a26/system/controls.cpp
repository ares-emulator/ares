auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  reset             = node->append<Node::Input::Button>("Reset");
  select            = node->append<Node::Input::Button>("Select");
  leftDifficulty    = node->append<Node::Input::Button>("Left Difficulty");
  rightDifficulty   = node->append<Node::Input::Button>("Right Difficulty");
  tvType            = node->append<Node::Input::Button>("TV Type");
}

auto System::Controls::poll() -> void {
  platform->input(reset);
  platform->input(select);
  platform->input(leftDifficulty);
  platform->input(rightDifficulty);
  platform->input(tvType);
}
