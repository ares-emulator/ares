auto System::Controls::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Object>(parent, from, "Controls");
  from = Node::scan(parent = node, from);

  reset      = Node::append<Node::Button>(parent, from, "Reset");
  microphone = Node::append<Node::Button>(parent, from, "Microphone");
}

auto System::Controls::poll() -> void {
  platform->input(reset);
  platform->input(microphone);
}
