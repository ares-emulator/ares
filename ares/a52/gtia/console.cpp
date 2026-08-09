auto GTIA::Console::power() -> void {
  *this = {};
  output = 0x07;
  pinSense = 0x08;
  for(auto& value : trigger) value = 1;
}

auto GTIA::Console::clock(n3 graphicsControl) -> void {
  // GTIA latches T0-T3 whenever an input goes low, independently of CPU reads.
  // Source: Atari GTIA Chip (NTSC) data sheet, section 7.0, Trigger Inputs.
  for(u32 index = 0; index < 4; index++) {
    auto state = !controllerPorts[index].bottomFire();
    if(graphicsControl.bit(2)) trigger[index] &= state;
    else trigger[index] = state;
  }
}

auto GTIA::Console::readTrigger(u8 index, n3 graphicsControl) -> n1 {
  clock(graphicsControl);
  return trigger[index];
}

auto GTIA::Console::triggerValue(u8 index) const -> n1 {
  return trigger[index];
}

auto GTIA::Console::pins() const -> n4 {
  return pinSense;
}

auto GTIA::Console::releaseTriggers() -> void {
  for(auto& value : trigger) value = 1;
}

auto GTIA::Console::write(n8 data) -> void {
  output = data;
  // A written one enables GTIA's sink transistor and reads back low. A zero
  // releases the pin to the board pull-up. S3 is only modeled as released-high;
  // its four-port-board destination is still unknown.
  pinSense = ~(u8)output;
}

auto GTIA::controllerSelect() const -> u32 {
  return (u8)console.output & 3;
}

auto GTIA::controllerPower() const -> bool {
  return console.output.bit(2);
}
