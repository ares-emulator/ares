auto VsUniSystem::Controls::ZapperInput::serialize(serializer& s) -> void {
  lightGun.serialize(s);
}

auto VsUniSystem::Controls::serialize(serializer& s) -> void {
  if(input) input->serialize(s);
}

auto VsUniSystem::DIPSwitches::serialize(serializer& s) -> void {
  s(value);
}

auto VsUniSystem::IO::serialize(serializer& s) -> void {
  s(streams);
  s(positions);
  s(strobe);
  s(servicePulse.held);
  s(servicePulse.frames);
  s(coinPulses[0].held);
  s(coinPulses[0].frames);
  s(coinPulses[1].held);
  s(coinPulses[1].frames);
}

auto VsUniSystem::serialize(serializer& s) -> void {
  s(controls);
  s(dipSwitches);
  s(io);
}
