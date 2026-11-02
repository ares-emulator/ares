auto Program::stateSave(u32 slot) -> bool {
  Program::Guard guard;
  if(!emulator) return false;

  auto location = emulator->locate(emulator->game->location, {".bs", slot}, settings.paths.saves);
  if(file::exists(location)) {
    auto undoLocation = emulator->locate(emulator->game->location, ".bsu", settings.paths.saves);
    file::remove(undoLocation);
    if(file::move(location, undoLocation)) {
      state.undoSlot = slot;
      presentation.revertSaveStateMenu.setEnabled(true);
    }
  }

  if(auto state = emulator->root->serialize()) {
    if(file::write(location, {state.data(), state.size()})) {
      presentation.refreshStateMenus();
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
    if(file::write(undoLocation, {state.data(), state.size()})) {
      presentation.undoLoadStateMenu.setEnabled(true);
    }
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

auto Program::revertStateSave() -> bool {
  Program::Guard guard;
  if(!emulator) return false;

  auto location = emulator->locate(emulator->game->location, {".bs", state.undoSlot}, settings.paths.saves);
  auto undoLocation = emulator->locate(emulator->game->location, ".bsu", settings.paths.saves);
  if(file::move(undoLocation, location)) {
    presentation.refreshStateMenus();
    showMessage({"Reverted to previous version in slot ", state.undoSlot, " of save file ", location});
    presentation.revertSaveStateMenu.setEnabled(false);
    return true;
  }

  showMessage({"Unable to revert to previous version of save file ", location});
  return false;
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
      presentation.undoLoadStateMenu.setEnabled(false);
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

  presentation.revertSaveStateMenu.setEnabled(false);
  presentation.undoLoadStateMenu.setEnabled(false);
}
