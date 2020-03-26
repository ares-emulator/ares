auto Emulator::saveState(uint slot) -> bool {
  if(!system.power) return false;
  if(auto cartridge = root->find<ares::Node::Peripheral>(0)) {
    if(auto location = cartridge->attribute("location")) {
      if(auto state = interface->serialize()) {
        directory::create({location, "State/"});
        if(file::write({location, "State/Slot ", slot, ".bst"}, {state.data(), state.size()})) {
          showMessage({"Saved state ", slot});
          return true;
        }
      }
    }
  }
  showMessage({"Failed to save state ", slot});
  return false;
}

auto Emulator::loadState(uint slot) -> bool {
  if(!system.power) return false;
  if(auto cartridge = root->find<ares::Node::Peripheral>(0)) {
    if(auto location = cartridge->attribute("location")) {
      if(auto memory = file::read({location, "State/Slot ", slot, ".bst"})) {
        serializer state{memory.data(), (uint)memory.size()};
        if(interface->unserialize(state)) {
          showMessage({"Loaded state ", slot});
          return true;
        }
      }
    }
  }
  showMessage({"Failed to load state ", slot});
  return false;
}
