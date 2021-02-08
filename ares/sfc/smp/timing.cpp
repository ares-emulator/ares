//DSP clock (~24576khz) / 12 (~2048khz) is fed into the SMP
//from here, the wait states value is really a clock divider of {2, 4, 8, 16}
//due to an unknown hardware issue, clock dividers of 8 and 16 are glitchy
//the SMP ends up consuming 10 and 20 clocks per opcode cycle instead
//this causes unpredictable behavior on real hardware
//sometimes the SMP will run far slower than expected
//other times (and more likely), the SMP will deadlock until the system is reset
//the timers are not affected by this and advance by their expected values
inline auto SMP::wait(bool halve, maybe<n16> address) -> void {
  static constexpr u32 cycleWaitStates[4] = {2, 4, 10, 20};
  static constexpr u32 timerWaitStates[4] = {2, 4,  8, 16};

  u32 waitStates = io.externalWaitStates;
  if(!address) waitStates = io.internalWaitStates;  //idle cycles
  else if((*address & 0xfff0) == 0x00f0) waitStates = io.internalWaitStates;  //IO registers
  else if(*address >= 0xffc0 && io.iplromEnable) waitStates = io.internalWaitStates;  //IPLROM

  step(cycleWaitStates[waitStates] >> halve);
  stepTimers(timerWaitStates[waitStates] >> halve);
}

inline auto SMP::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
  Thread::synchronize(dsp);
}

inline auto SMP::stepTimers(u32 clocks) -> void {
  timer0.step(clocks);
  timer1.step(clocks);
  timer2.step(clocks);
}

template<u32 Frequency> auto SMP::Timer<Frequency>::step(u32 clocks) -> void {
  //stage 0 increment
  stage0 += clocks;
  if(stage0 < Frequency) return;
  stage0 -= Frequency;

  //stage 1 increment
  stage1 ^= 1;
  synchronizeStage1();
}

template<u32 Frequency> auto SMP::Timer<Frequency>::synchronizeStage1() -> void {
  bool level = stage1;
  if(!smp.io.timersEnable) level = false;
  if(smp.io.timersDisable) level = false;
  if(!line.lower(level)) return;  //only pulse on 1->0 transition

  //stage 2 increment
  if(!enable) return;
  if(++stage2 != target) return;

  //stage 3 increment
  stage2 = 0;
  stage3++;
}
