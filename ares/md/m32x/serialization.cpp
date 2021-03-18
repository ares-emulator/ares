auto M32X::serialize(serializer& s) -> void {
  s(sdram);
  s(shm);
  s(shs);
  s(vdp);
  s(pwm);
}

auto M32X::SH7604::serialize(serializer& s) -> void {
  Thread::serialize(s);
  SH2::serialize(s);
}

auto M32X::VDP::serialize(serializer& s) -> void {
  s(dram);
  s(cram);
}

auto M32X::PWM::serialize(serializer& s) -> void {
  Thread::serialize(s);
}
