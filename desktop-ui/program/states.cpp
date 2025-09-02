auto Program::stateSave(u32 slot) -> bool {
  Program::Guard guard;
  if(!emulator) return false;

  auto location = emulator->locate(emulator->game->location, {".bs", slot}, settings.paths.saves);
  string undoLocation = {location.slice(0, (location.size() - 1)), "u"};
  if(file::move(location, undoLocation)) {
    state.undoSlot = slot;
  }

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
  Program::Guard guard;
  if(!emulator) return false;

  //Store current state for undo
  auto undoLocation = emulator->locate(emulator->game->location, {".blu"}, settings.paths.saves);
  if(auto state = emulator->root->serialize()) {
    file::write(undoLocation, {state.data(), state.size()});
  }

  auto location = emulator->locate(emulator->game->location, {".bs", slot}, settings.paths.saves);
  auto memory = file::read(location);
  if(!memory.empty()) {
    serializer state{memory.data(), (u32)memory.size()};
    if(emulator->root->unserialize(state)) {
      showMessage({"Loaded state from slot ", slot});
      return true;
    }
  }

  showMessage({"Failed to load state from slot ", slot});
  return false;
}

auto Program::undoStateSave() -> bool {
  Program::Guard guard;
  if(!emulator) return false;

  auto undoLocation = emulator->locate(emulator->game->location, ".bsu", settings.paths.saves);
  string location = {undoLocation.slice(0, (undoLocation.size() - 1)), state.undoSlot};
  if(file::move(undoLocation, location)) {
    showMessage({"Reverted to previous version in slot ", state.undoSlot, " of save file ", location});
      return true;
  } else {
    showMessage({"Unable to revert to previous version of save file ", location});
    return false;
  }
}

auto Program::undoStateLoad() -> bool {
  Program::Guard guard;
  if(!emulator) return false;

  auto undoLocation = emulator->locate(emulator->game->location, ".blu", settings.paths.saves);
  auto memory = file::read(undoLocation);
  if(!memory.empty()) {
    serializer state{memory.data(), (u32)memory.size()};
    if(emulator->root->unserialize(state)) {
      showMessage({"Loaded state from undo load file ", undoLocation});
      file::remove(undoLocation);
      return true;
    } else {
      showMessage({"Failed to unserialize state from undo load file ", undoLocation});
      return false;
    }
  } else {
    showMessage({"Unable to revert to previous state from undo load file ", undoLocation});
    return false;
  }
}

auto Program::clearUndoStates() -> void {
  Program::Guard guard;
  if(!emulator) return;

  auto location = emulator->locate(emulator->game->location, ".blu", settings.paths.saves);
  file::remove(location);

  location = emulator->locate(emulator->game->location, ".bsu", settings.paths.saves);
  file::remove(location);
}
