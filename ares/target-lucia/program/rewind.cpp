auto Program::rewindSetMode(Rewind::Mode mode) -> void {
  rewind.mode = mode;
  rewind.counter = 0;
}

auto Program::rewindReset() -> void {
  rewindSetMode(Rewind::Mode::Playing);
  rewind.history.reset();
  rewind.length = settings.rewind.length;
  rewind.frequency = settings.rewind.frequency;
}

auto Program::rewindRun() -> void {
  if(rewind.frequency == 0) return;  //rewind disabled?

  if(rewind.mode == Rewind::Mode::Playing) {
    if(++rewind.counter < rewind.frequency) return;
    rewind.counter = 0;
    if(rewind.history.size() >= rewind.length) rewind.history.takeFirst();
    auto s = emulator->interface->serialize(0);
    rewind.history.append(s);
  }

  if(rewind.mode == Rewind::Mode::Rewinding) {
    if(!rewind.history.size()) return rewindSetMode(Rewind::Mode::Playing);  //nothing left to rewind?
    if(++rewind.counter < rewind.frequency / 5) return;  //rewind 5x faster than playing
    rewind.counter = 0;
    auto s = rewind.history.takeLast();
    s.setMode(serializer::Mode::Load);
    emulator->interface->unserialize(s);
    if(!rewind.history) {
      showMessage("Rewind history exhausted");
      rewindReset();
    }
  }
}
