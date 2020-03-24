auto SMP::serialize(serializer& s) -> void {
  SPC700::serialize(s);
  Thread::serialize(s);

  s.integer(io.clockCounter);
  s.integer(io.dspCounter);

  s.integer(io.apu0);
  s.integer(io.apu1);
  s.integer(io.apu2);
  s.integer(io.apu3);

  s.integer(io.timersDisable);
  s.integer(io.ramWritable);
  s.integer(io.ramDisable);
  s.integer(io.timersEnable);
  s.integer(io.externalWaitStates);
  s.integer(io.internalWaitStates);

  s.integer(io.iplromEnable);

  s.integer(io.dspAddress);

  s.integer(io.cpu0);
  s.integer(io.cpu1);
  s.integer(io.cpu2);
  s.integer(io.cpu3);

  s.integer(io.aux4);
  s.integer(io.aux5);

  timer0.serialize(s);
  timer1.serialize(s);
  timer2.serialize(s);
}

template<uint Frequency>
auto SMP::Timer<Frequency>::serialize(serializer& s) -> void {
  s.integer(stage0);
  s.integer(stage1);
  s.integer(stage2);
  s.integer(stage3);
  s.boolean(line);
  s.boolean(enable);
  s.integer(target);
}
