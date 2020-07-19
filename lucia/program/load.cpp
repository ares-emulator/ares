auto Program::identify(const string& filename) -> shared_pointer<Emulator> {
  if(auto system = mia::identify(filename)) {
    for(auto& emulator : emulators) {
      if(emulator->name == system) return emulator;
    }
  }

  MessageDialog().setTitle("lucia").setText({
    "Filename: ", Location::file(filename), "\n\n"
    "Unable to determine what type of game this file is.\n"
    "Please use the load menu to choose the appropriate game system instead."
  }).setAlignment(presentation).error();
  return {};
}

auto Program::load(shared_pointer<Emulator> emulator, string filename) -> bool {
  if(!filename) {
    string location = emulator->configuration.game;
    if(!location) location = Path::desktop();

    BrowserDialog dialog;
    dialog.setTitle({"Load ", emulator->name, " Game"});
    dialog.setPath(location);
    dialog.setAlignment(presentation);
    string filters{"*.zip:"};
    for(auto& extension : emulator->medium->extensions()) {
      filters.append("*.", extension, ":");
    }
    //support both uppercase and lowercase extensions
    filters.append(string{filters}.upcase());
    filters.trimRight(":", 1L);
    filters.prepend(emulator->name, " Games|");
    dialog.setFilters({filters, "All Files|*"});
    filename = openFile(dialog);
  }
  if(!file::exists(filename)) return false;

  vector<uint8_t> filedata;
  if(filename.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(filename)) {
      for(auto& file : archive.file) {
        auto extension = Location::suffix(file.name).trimLeft(".", 1L).downcase();
        if(emulator->medium->extensions().find(extension)) {
          filedata = archive.extract(file);
          break;
        }
      }
    }
  } else {
    filedata = file::read(filename);
  }
  if(!filedata) return false;

  //apply patch (if one exists)
  bool patchApplied = false;
  if(auto patch = file::read(emulator->locate(filename, ".bps", settings.paths.patches))) {
    if(auto output = Beat::Single::apply(filedata, patch)) {
      filedata = output();
      patchApplied = true;
    }
  }

  unload();
  ::emulator = emulator;
  if(!emulator->load(filename, filedata)) {
    ::emulator.reset();
    if(settings.video.adaptiveSizing) presentation.resizeWindow();
    presentation.showIcon(true);
    return false;
  }

  //this is a safeguard warning in case the user loads their games from a read-only location:
  string savesPath = settings.paths.saves;
  if(!savesPath) savesPath = Location::path(filename);
  if(!directory::writable(savesPath)) {
    MessageDialog().setTitle("lucia").setText({
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
  if(settings.general.autoDebug) {
    pause(true);
    toolsWindow.show("Tracer");
    presentation.setFocused();
  } else {
    pause(false);
  }
  showMessage({"Loaded ", Location::prefix(emulator->game.location), patchApplied ? ", and patch applied" : ""});

  //update recent games list
  for(int index = 7; index >= 0; index--) {
    settings.recent.game[index + 1] = settings.recent.game[index];
  }
  settings.recent.game[0] = {emulator->name, ";", filename};
  presentation.loadEmulators();

  return true;
}

auto Program::unload() -> void {
  if(!emulator) return;

  settings.save();
  showMessage({"Unloaded ", Location::prefix(emulator->game.location)});
  emulator->unload();
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
