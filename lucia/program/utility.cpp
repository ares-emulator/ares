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
  for(auto& screen : emulator->root->find<ares::Node::Screen>()) {
    screen->setLuminance(settings.video.luminance);
    screen->setSaturation(settings.video.saturation);
    screen->setGamma(settings.video.gamma);
  }
}

auto Program::runAheadUpdate() -> void {
  runAhead = settings.general.runAhead;
  if(emulator && emulator->name == "Game Boy Advance") runAhead = false;  //crashes immediately
  if(emulator && emulator->name == "MSX") runAhead = false;  //unstable
  if(emulator && emulator->name == "MSX2") runAhead = false;  //unstable
}

auto Program::captureScreenshot(const uint32_t* data, uint pitch, uint width, uint height) -> void {
  string filename{emulator->locate({Location::notsuffix(emulator->game.location), " ", chrono::local::datetime().transform(":", "-"), ".png"}, ".png", settings.paths.screenshots)};
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
    return window.open();
  }
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
