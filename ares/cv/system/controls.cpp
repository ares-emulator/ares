auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  reset = node->append<Node::Button>("Reset");
}

auto System::Controls::poll() -> void {
  auto value = reset->value();
  platform->input(reset);
  if(!value && reset->value()) system.power();  //todo: implement soft reset
}
