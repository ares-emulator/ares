auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  pause = node->append<Node::Button>("Pause");
}

auto System::Controls::poll() -> void {
  auto value = pause->value();
  platform->input(pause);
  if(!value && pause->value()) cpu.setNMI(1);
}
