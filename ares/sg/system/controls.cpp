auto System::Controls::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Object>(parent, from, "Controls");
  from = Node::scan(parent = node, from);

  pause = Node::append<Node::Button>(parent, from, "Pause");
}

auto System::Controls::poll() -> void {
  auto value = pause->value();
  platform->input(pause);
  if(!value && pause->value()) cpu.setNMI(1);
}
