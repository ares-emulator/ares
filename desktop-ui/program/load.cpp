auto Program::identify(const string& filename) -> shared_pointer<Emulator> {
  Program::Guard guard;
  if(auto system = mia::identify(filename)) {
    for(auto& emulator : emulators) {
      if(emulator->name == system) return emulator;
    }
  }

  MessageDialog().setTitle(ares::Name).setText({
    "Filename: ", Location::file(filename), "\n\n"
    "Unable to determine what type of game this file is.\n"
    "Please use the load menu to choose the appropriate game system instead."
  }).setAlignment(presentation).error();
  return {};
}

/// Loads an emulator and, optionally, a ROM from the given location.
auto Program::load(shared_pointer<Emulator> emulator, string location) -> bool {
  Program::Guard guard;
  unload();

  ::emulator = emulator;

  // For arcade systems, show the game browser dialog as we're using MAME-compatible roms
  if(emulator->arcade() && !location) {
    gameBrowserWindow.show(emulator);
    
    // Temporarily pretend that the load failed to prevent crash
    // The browser dialog will call load() again when necessary
    ::emulator.reset();
    return false;
  }

  return load(location);
}

/// Loads a ROM for an already-loaded emulator.
auto Program::load(string location) -> bool {
  Program::Guard guard;
  if(settings.debugServer.enabled) {
    nall::GDB::server.reset();
  }

  if(!emulator->load(location)) {
    emulator.reset();
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
    presentation.showIcon(true);
    return false;
  }
  location = emulator->game->location;

  //this is a safeguard warning in case the user loads their games from a read-only location:
  string savesPath = settings.paths.saves;
  if(!savesPath) savesPath = Location::path(location);
  if(!directory::writable(savesPath)) {
    MessageDialog().setTitle(ares::Name).setText({
      "The current save path is read-only; please choose a writable save path now.\n"
      "Otherwise, any in-game progress will be lost once this game is unloaded!\n\n"
      "Current save location: ", savesPath
    }).warning();
  }

  paletteUpdate();
  runAheadUpdate();
  presentation.loadEmulator();
  presentation.showIcon(false);
  if(settings.video.adaptiveSizing) presentation.resizeWindow();
  manifestViewer.reload();
  cheatEditor.reload();
  memoryEditor.reload();
  graphicsViewer.reload();
  streamManager.reload();
  propertiesViewer.reload();
  traceLogger.reload();
  state = {};  //reset hotkey state slot to 1
  if(settings.boot.debugger) {
    pause(true);
    toolsWindow.show("Tracer");
    presentation.setFocused();
  } else if (settings.boot.waitGDB) {
    pause(true);
  } else {
    pause(false);
  }

  showMessage({"Loaded ", Location::prefix(location)});

  if(settings.debugServer.enabled) {
    nall::GDB::server.open(settings.debugServer.port, settings.debugServer.useIPv4);
    nall::GDB::server.onClientConnectCallback = []() {
      if (settings.boot.waitGDB)
        program.pause(false);
    };
  }

  //update recent games list
  for(s32 index = 7; index >= 0; index--) {
    settings.recent.game[index + 1] = settings.recent.game[index];
  }
  settings.recent.game[0] = {emulator->name, ";", location};
  presentation.loadEmulators();

  configuration = emulator->root->attribute("configuration");

  return true;
}

auto Program::unload() -> void {
  Program::Guard guard;
  if(!emulator) return;

  nall::GDB::server.close();
  nall::GDB::server.reset();

  settings.save();
  clearUndoStates();
  showMessage({"Unloaded ", Location::prefix(emulator->game->location)});
  emulator->unload();
  screens.clear();
  streams.clear();
  emulator.reset();
  rewindReset();
  presentation.unloadEmulator();
  toolsWindow.setVisible(false);
  gameBrowserWindow.setVisible(false);
  manifestViewer.unload();
  cheatEditor.unload();
  memoryEditor.unload();
  graphicsViewer.unload();
  streamManager.unload();
  propertiesViewer.unload();
  traceLogger.unload();
  message.text = "";
  configuration = "";
  ruby::video.clear();
  ruby::audio.clear();
}
