auto VsUniSystem::Controls::load(Node::Object parent) -> bool {
  unload();
  if(!cartridge.pak) return false;

  auto mode = cartridge.pak->attribute("input");
  if(mode != "standard" && mode != "swapped" && mode != "swap-ab" && mode != "zapper") return false;

  node = parent->append<Node::Object>("Controls");
  service = node->append<Node::Input::Button>("Service");
  coins[0] = node->append<Node::Input::Button>("Player 1 Coin");
  coins[1] = node->append<Node::Input::Button>("Player 2 Coin");

  n8 fixedBits = cartridge.pak->attribute("protection") == "ice-climber" ? 0x08 : 0x00;
  if(mode == "standard") input = std::make_unique<StandardInput>(node, StandardInput::Wiring::Standard, fixedBits);
  if(mode == "swapped")  input = std::make_unique<StandardInput>(node, StandardInput::Wiring::Swapped, fixedBits);
  if(mode == "swap-ab")  input = std::make_unique<StandardInput>(node, StandardInput::Wiring::SwapAB, fixedBits);
  if(mode == "zapper")   input = std::make_unique<ZapperInput>(node);
  return true;
}

auto VsUniSystem::Controls::unload() -> void {
  input.reset();
  service.reset();
  coins[0].reset();
  coins[1].reset();
  if(auto parent = Node::parent(node)) parent->remove(node);
  node.reset();
}

auto VsUniSystem::Controls::data(u32 stream) -> n1 {
  if(input) return input->data(stream);
  return 0;
}

auto VsUniSystem::Controls::latch() -> std::array<n8, 2> {
  if(input) return input->latch();
  return {};
}
