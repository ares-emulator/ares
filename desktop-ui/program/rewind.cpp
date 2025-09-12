auto Program::rewindSetMode(Rewind::Mode mode) -> void {
  Program::Guard guard;
  rewind.mode = mode;
  rewind.counter = 0;
}

auto Program::rewindReset() -> void {
  Program::Guard guard;
  rewindSetMode(Rewind::Mode::Playing);
  rewind.history.clear();
  rewind.length = settings.rewind.length;
  rewind.frequency = settings.rewind.frequency;
}

auto Program::rewindRun() -> void {
  if(!settings.general.rewind) return;  //rewind disabled?
  Program::Guard guard;

  if(rewind.mode == Rewind::Mode::Playing) {
    if(++rewind.counter < rewind.frequency) return;
    rewind.counter = 0;
    if(rewind.history.size() >= rewind.length) rewind.history.erase(rewind.history.begin());
    auto s = emulator->root->serialize(0);
    rewind.history.push_back(s);
  }

  if(rewind.mode == Rewind::Mode::Rewinding) {
    if(!rewind.history.size()) return rewindSetMode(Rewind::Mode::Playing);  //nothing left to rewind?
    if(++rewind.counter < rewind.frequency / 5) return;  //rewind 5x faster than playing
    rewind.counter = 0;
    auto s = rewind.history.back();
    rewind.history.pop_back();
    s.setReading();
    emulator->root->unserialize(s);
    if(rewind.history.empty()) {
      showMessage("Rewind history exhausted");
      rewindReset();
    }
  }
}
