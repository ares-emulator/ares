auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  reset = node->append<Node::Input::Button>("Reset");
  if(system.region() != System::Region::Dendy) {
    microphone = node->append<Node::Input::Button>("Microphone");
  }
}

auto System::Controls::unload() -> void {
  microphone.reset();
  reset.reset();
  node.reset();
}

auto System::Controls::poll() -> void {
  platform->input(reset);
  if(microphone) platform->input(microphone);
}
