auto SMP::serialize(serializer& s) -> void {
  SPC700::serialize(s);
  Thread::serialize(s);

  s(io.clockCounter);
  s(io.dspCounter);

  s(io.apu0);
  s(io.apu1);
  s(io.apu2);
  s(io.apu3);

  s(io.timersDisable);
  s(io.ramWritable);
  s(io.ramDisable);
  s(io.timersEnable);
  s(io.externalWaitStates);
  s(io.internalWaitStates);

  s(io.iplromEnable);

  s(io.dspAddress);

  s(io.cpu0);
  s(io.cpu1);
  s(io.cpu2);
  s(io.cpu3);

  s(io.aux4);
  s(io.aux5);

  s(timer0);
  s(timer1);
  s(timer2);
}

template<u32 Frequency>
auto SMP::Timer<Frequency>::serialize(serializer& s) -> void {
  s(stage0);
  s(stage1);
  s(stage2);
  s(stage3);
  s(line);
  s(enable);
  s(target);
}
