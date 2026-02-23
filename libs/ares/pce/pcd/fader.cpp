auto PCD::Fader::clock() -> void {
  if(mode != Mode::Idle) {
    volume -= step;
    if(volume <= 0.0) volume = 0.0;
  }
}

auto PCD::Fader::power() -> void {
  mode = Mode::Idle;
  step = 0.0;
  volume = 1.0;
}
