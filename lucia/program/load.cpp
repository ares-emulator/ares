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

auto Program::load(shared_pointer<Emulator> emulator, string location) -> bool {
  if(!location) {
    string pathname = emulator->configuration.game;
    if(!pathname) pathname = Path::desktop();

    BrowserDialog dialog;
    dialog.setTitle({"Load ", emulator->name, " Game"});
    dialog.setPath(pathname);
    dialog.setAlignment(presentation);
    string filters{"*.zip:"};
    for(auto& extension : emulator->medium->extensions()) {
      filters.append("*.", extension, ":");
    }
    //support both uppercase and lowercase extensions
    filters.append(string{filters}.upcase());
    filters.trimRight(":", 1L);
    filters.prepend(emulator->name, "|");
    dialog.setFilters({filters, "All|*"});
    location = openFile(dialog);
  }
  if(!inode::exists(location)) return false;

  unload();
  ::emulator = emulator;
  if(!emulator->load(location)) {
    ::emulator.reset();
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
    presentation.showIcon(true);
    return false;
  }

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
  showMessage({"Loaded ", Location::prefix(emulator->game.location)});

  //update recent games list
  for(s32 index = 7; index >= 0; index--) {
    settings.recent.game[index + 1] = settings.recent.game[index];
  }
  settings.recent.game[0] = {emulator->name, ";", location};
  presentation.loadEmulators();

  return true;
}

auto Program::load(shared_pointer<mia::Media> medium, string& path) -> string {
  BrowserDialog dialog;
  dialog.setTitle({"Load ", medium->name(), " Game"});
  dialog.setPath(path ? path : Path::desktop());
  dialog.setAlignment(presentation);
  string filters{"*.zip:"};
  for(auto& extension : medium->extensions()) {
    filters.append("*.", extension, ":");
  }
  //support both uppercase and lowercase extensions
  filters.append(string{filters}.upcase());
  filters.trimRight(":", 1L);
  filters.prepend(medium->name(), "|");
  dialog.setFilters({filters, "All|*"});
  string location = openFile(dialog);
  if(location) path = Location::dir(location);
  return location;
}

auto Program::unload() -> void {
  if(!emulator) return;

  settings.save();
  showMessage({"Unloaded ", Location::prefix(emulator->game.location)});
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
