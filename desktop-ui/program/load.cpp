auto Program::identify(const string& filename) -> shared_pointer<Emulator> {
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

//location is an optional game to load automatically (for command-line loading)
auto Program::load(shared_pointer<Emulator> emulator, string location) -> bool {
  unload();

  ::emulator = emulator;
  if(!emulator->load(location)) {
    ::emulator.reset();
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
  memoryEditor.reload();
  graphicsViewer.reload();
  streamManager.reload();
  propertiesViewer.reload();
  traceLogger.reload();
  state = {};  //reset hotkey state slot to 1
  if(settings.boot.debugger) {
    pause(true);
    traceLogger.traceToTerminal.setChecked(true);
    traceLogger.traceToFile.setChecked(false);
    toolsWindow.show("Tracer");
    presentation.setFocused();
  } else {
    pause(false);
  }
  showMessage({"Loaded ", Location::prefix(location)});

  //update recent games list
  for(s32 index = 7; index >= 0; index--) {
    settings.recent.game[index + 1] = settings.recent.game[index];
  }
  settings.recent.game[0] = {emulator->name, ";", location};
  presentation.loadEmulators();

  return true;
}

auto Program::unload() -> void {
  if(!emulator) return;

  settings.save();
  showMessage({"Unloaded ", Location::prefix(emulator->game->location)});
  emulator->unload();
  screens.reset();
  streams.reset();
  emulator.reset();
  rewindReset();
  presentation.unloadEmulator();
  toolsWindow.setVisible(false);
  manifestViewer.unload();
  memoryEditor.unload();
  graphicsViewer.unload();
  streamManager.unload();
  propertiesViewer.unload();
  traceLogger.unload();
  message.text = "";
  ruby::video.clear();
  ruby::audio.clear();
}
