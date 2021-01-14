auto SPU::Envelope::reset(u8 rate, bool decreasing, bool exponential) -> void {
  this->rate = rate;
  this->decreasing = decreasing;
  this->exponential = exponential;
  auto& entry = spu.adsrEntries[decreasing][rate];
  this->counter = entry.ticks;
}

auto SPU::Envelope::tick(s32 level) -> s16 {
  if(--counter > 0) return level;
  auto& entry = spu.adsrEntries[decreasing][rate];
  counter = entry.ticks;
  s32 step = entry.step;
  if(exponential) {
    if(decreasing) {
      step = amplify(step, level);
    } else if(level >= 0x6000) {
      if(rate < 40) {
        step >>= 2;
      } else if(rate >= 44) {
        counter >>= 2;
      } else {
        step >>= 1;
        counter >>= 1;
      }
    }
  }
  level += step;
  if(level < 0x0000) level = 0x0000;
  if(level > 0x7fff) level = 0x7fff;
  return level;
}

auto SPU::VolumeSweep::reset() -> void {
  if(sweep == 0) {
    active = 0;
    current = level << 1;
  } else {
    active = 1;
    Envelope::reset(rate, decreasing, exponential);
  }
}

auto SPU::VolumeSweep::tick() -> void {
  if(active) {
    current = Envelope::tick(current);
    active = decreasing ? current > 0x0000 : current < 0x7fff;
  }
}
