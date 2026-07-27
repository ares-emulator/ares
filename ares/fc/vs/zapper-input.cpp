VsUniSystem::Controls::ZapperInput::ZapperInput(Node::Object parent) {
  node = parent->append<Node::Object>("Zapper");
  x = node->append<Node::Input::Axis>("X-Axis");
  y = node->append<Node::Input::Axis>("Y-Axis");
  trigger = node->append<Node::Input::Button>("Trigger");
  lightGun.load(node, x, y, trigger);
}

VsUniSystem::Controls::ZapperInput::~ZapperInput() {
  lightGun.unload();
}

auto VsUniSystem::Controls::ZapperInput::data(u32) -> n1 {
  return 0;
}

auto VsUniSystem::Controls::ZapperInput::latch() -> std::array<n8, 2> {
  auto gun = lightGun.data();
  n8 stream = 0;
  stream.bit(4) = 1;  //Gun alarm wire connected
  stream.bit(6) = !gun.bit(1);
  stream.bit(7) = gun.bit(2);
  return {stream, 0};
}
