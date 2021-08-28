auto Program::pause(bool state) -> void {
  if(paused == state) return;
  paused = state;
  presentation.pauseEmulation.setChecked(paused);

  if(paused) {
    ruby::audio.clear();
    presentation.statusRight.setText("Paused");
  }
}

auto Program::paletteUpdate() -> void {
  if(!emulator) return;
  for(auto& screen : emulator->root->find<ares::Node::Video::Screen>()) {
    screen->setLuminance(settings.video.luminance);
    screen->setSaturation(settings.video.saturation);
    screen->setGamma(settings.video.gamma);
  }
}

auto Program::runAheadUpdate() -> void {
  runAhead = settings.general.runAhead;
  if(!emulator) return;
  if(emulator->name == "Game Boy Advance") runAhead = false;  //crashes immediately
  if(emulator->name == "Nintendo 64") runAhead = false;  //too demanding
  if(emulator->name == "PlayStation") runAhead = false;  //too demanding
}

auto Program::captureScreenshot(const u32* data, u32 pitch, u32 width, u32 height) -> void {
  string filename{emulator->locate({Location::notsuffix(emulator->game->location), " ", chrono::local::datetime().transform(":", "-"), ".png"}, ".png", settings.paths.screenshots)};
  if(Encode::PNG::RGB8(filename, data, pitch, width, height)) {
    showMessage("Captured screenshot");
  } else {
    showMessage("Failed to capture screenshot");
  }
}

auto Program::openFile(BrowserDialog& dialog) -> string {
  if(settings.general.nativeFileDialogs) {
    BrowserWindow window;
    window.setTitle(dialog.title());
    window.setPath(dialog.path());
    window.setFilters(dialog.filters());
    window.setParent(dialog.alignmentWindow());
    // Only affects macOS. TODO: are there scenarios where we want to forbid selecting folders?
    window.setAllowsFolders(true);
    return window.open();
  }
  if(dialog.filters().size() > 1) return dialog.openObject();
  return dialog.openFile();
}

auto Program::selectFolder(BrowserDialog& dialog) -> string {
  if(settings.general.nativeFileDialogs) {
    BrowserWindow window;
    window.setTitle(dialog.title());
    window.setPath(dialog.path());
    window.setParent(dialog.alignmentWindow());
    return window.directory();
  }
  return dialog.selectFolder();
}
