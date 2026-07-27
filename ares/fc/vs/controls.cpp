auto VsUniSystem::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");
  service = node->append<Node::Input::Button>("Service");
  coins[0] = node->append<Node::Input::Button>("Player 1 Coin");
  coins[1] = node->append<Node::Input::Button>("Player 2 Coin");
}

auto VsUniSystem::Controls::unload() -> void {
  disconnect();
  service.reset();
  coins[0].reset();
  coins[1].reset();
  node.reset();
}

auto VsUniSystem::Controls::connect() -> bool {
  if(!cartridge.pak) return false;

  auto mode = cartridge.pak->attribute("input");
  if(mode == "standard") input = std::make_unique<StandardInput>(node, StandardInput::Wiring::Standard);
  if(mode == "swapped")  input = std::make_unique<StandardInput>(node, StandardInput::Wiring::Swapped);
  if(mode == "swap-ab")  input = std::make_unique<StandardInput>(node, StandardInput::Wiring::SwapAB);
  if(mode == "zapper")   input = std::make_unique<ZapperInput>(node);
  return (bool)input;
}

auto VsUniSystem::Controls::disconnect() -> void {
  if(!input) return;
  auto inputNode = input->node;
  input.reset();
  if(node && inputNode) node->remove(inputNode);
}

auto VsUniSystem::Controls::data(u32 stream) -> n1 {
  if(input) return input->data(stream);
  return 0;
}

auto VsUniSystem::Controls::latch() -> std::array<n8, 2> {
  if(input) return input->latch();
  return {};
}
