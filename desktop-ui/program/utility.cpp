auto Program::pause(bool state) -> void {
  Program::Guard guard;
  if(paused == state) return;
  paused = state;
  presentation.pauseEmulation.setChecked(paused);

  if(paused) {
    ruby::audio.clear();
    presentation.statusRight.setText("Paused");
  }
}

auto Program::mute() -> void {
  Program::Guard guard;
  settings.audio.mute = !settings.audio.mute;
  presentation.muteAudioSetting.setChecked(settings.audio.mute);
}

auto Program::paletteUpdate() -> void {
  Program::Guard guard;
  if(!emulator) return;
  if(!emulator->root) return;
  for(auto& screen : emulator->root->find<ares::Node::Video::Screen>()) {
    screen->setLuminance(settings.video.luminance);
    screen->setSaturation(settings.video.saturation);
    screen->setGamma(settings.video.gamma);
  }
}

auto Program::runAheadUpdate() -> void {
  Program::Guard guard;
  runAhead = settings.general.runAhead;
  if(!emulator) return;
  if(emulator->name == "Game Boy Advance") runAhead = false;  //crashes immediately
  if(emulator->name == "Neo Geo Pocket") runAhead = false;  //white screen
  if(emulator->name == "Neo Geo Pocket Color") runAhead = false;  //white screen
  if(emulator->name == "Nintendo 64") runAhead = false;  //too demanding
  if(emulator->name == "Nintendo 64DD") runAhead = false;  //too demanding
  if(emulator->name == "PlayStation") runAhead = false;  //too demanding
  if(emulator->name == "SuperGrafx CD") runAhead = false; //too demanding
  if(emulator->name == "Mega 32X") runAhead = false;  //too demanding
  if(emulator->name == "Mega CD 32X") runAhead = false;  //too demanding
  if(emulator->name == "Sega Saturn") runAhead = false;  //too demanding
  if(emulator->name == "Arcade" && emulator->game->pak->attribute("board") == "nintendo/aleck64") runAhead = false;  //hangs, too demanding
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
  Program::Guard guard;
  BrowserWindow window;
  window.setTitle(dialog.title());
  window.setPath(dialog.path());
  window.setFilters(dialog.filters());
  window.setParent(dialog.alignmentWindow());
  // Only affects macOS. TODO: are there scenarios where we want to forbid selecting folders?
  window.setAllowsFolders(true);
  return window.open();
}

auto Program::selectFolder(BrowserDialog& dialog) -> string {
  Program::Guard guard;
  BrowserWindow window;
  window.setTitle(dialog.title());
  window.setPath(dialog.path());
  window.setParent(dialog.alignmentWindow());
  return window.directory();
}
