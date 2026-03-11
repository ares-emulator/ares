auto PIF::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(ram);
  s(state);
  s(intram);
  s(io.romLockout);
  s(io.resetEnabled);
}

auto PIF::Intram::serialize(serializer& s) -> void {
  for(auto& os : osInfo) s(os);
  for(auto& cpu : cpuChecksum) s(cpu);
  for(auto& cic : cicChecksum) s(cic);
  s(bootTimeout);
  for(auto& joy : joyAddress) s(joy);
  for(auto i: range(5)) s(joyStatus[i].skip), s(joyStatus[i].reset);
}
