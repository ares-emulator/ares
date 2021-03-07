auto Program::stateSave(u32 slot) -> bool {
  if(!emulator) return false;

  auto location = emulator->locate(emulator->game->location, {".bs", slot}, settings.paths.saves);
  if(auto state = emulator->root->serialize()) {
    if(file::write(location, {state.data(), state.size()})) {
      showMessage({"Saved state to slot ", slot});
      return true;
    }
  }

  showMessage({"Failed to save state to slot ", slot});
  return false;
}

auto Program::stateLoad(u32 slot) -> bool {
  if(!emulator) return false;

  auto location = emulator->locate(emulator->game->location, {".bs", slot}, settings.paths.saves);
  if(auto memory = file::read(location)) {
    serializer state{memory.data(), (u32)memory.size()};
    if(emulator->root->unserialize(state)) {
      showMessage({"Loaded state from slot ", slot});
      return true;
    }
  }

  showMessage({"Failed to load state from slot ", slot});
  return false;
}
