auto VsUniSystem::IO::readStream(u32 stream) -> n1 {
  if(strobe) return vsUniSystem.controls.data(stream);
  if(positions[stream] >= 8) return 1;
  return streams[stream].bit(positions[stream]++);
}

auto VsUniSystem::IO::read(n16 address, n8 data) -> n8 {
  data.bit(0) = readStream(address == 0x4017);
  data.bit(1) = 0;

  if(address == 0x4016) {
    data.bit(2) = servicePulse.frames != 0;
    data.bit(3, 4) = vsUniSystem.dipSwitches.value.bit(0, 1);
    data.bit(5) = coinPulses[0].frames != 0;
    data.bit(6) = coinPulses[1].frames != 0;
    data.bit(7) = 0;
  } else {
    data.bit(2, 7) = vsUniSystem.dipSwitches.value.bit(2, 7);
  }

  return data;
}

auto VsUniSystem::IO::write(n8 data) -> void {
  n1 nextStrobe = data.bit(0);
  if(strobe && !nextStrobe) {
    auto latched = vsUniSystem.controls.latch();
    streams[0] = latched[0];
    streams[1] = latched[1];
    positions[0] = 0;
    positions[1] = 0;
  }
  strobe = nextStrobe;
}

auto VsUniSystem::IO::pollPulse(Node::Input::Button input, Pulse& pulse) -> void {
  if(pulse.frames) pulse.frames--;
  platform->input(input);
  bool pressed = input->value();
  if(pressed && !pulse.held) pulse.frames = 3;
  pulse.held = pressed;
}

auto VsUniSystem::IO::frame() -> void {
  auto& controls = vsUniSystem.controls;
  pollPulse(controls.service, servicePulse);
  pollPulse(controls.coins[0], coinPulses[0]);
  pollPulse(controls.coins[1], coinPulses[1]);
}

auto VsUniSystem::IO::power() -> void {
  streams[0] = 0;
  streams[1] = 0;
  positions[0] = 0;
  positions[1] = 0;
  strobe = 0;
  servicePulse = {};
  coinPulses[0] = {};
  coinPulses[1] = {};
}
