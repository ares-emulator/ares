StandardController::StandardController(Node::Port parent) : Controller(parent, "Controller") {
  x = node->append<Node::Input::Axis>("X-Axis");
  y = node->append<Node::Input::Axis>("Y-Axis");
  appendControls();
}

auto StandardController::axis(u32 index, bool powered) -> s16 {
  if(!powered) return 32767;
  auto input = index == 0 ? x : y;
  if(platform) platform->input(input);
  return input->value();
}
