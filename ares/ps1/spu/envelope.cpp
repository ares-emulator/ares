auto SPU::Envelope::reset(u8 rate, bool decreasing, bool exponential, u8 rateMask) -> void {
  this->rate = rate;
  this->decreasing = decreasing;
  this->exponential = exponential;
  counter = rate >= 48 && (rate & rateMask) == rateMask ? -1 : 0;
}

auto SPU::Envelope::tick(s32 level) -> s16 {
  if(counter < 0) return level;
  auto& entry = spu.adsrEntries[decreasing][rate];
  s32 increment = entry.increment;
  s32 step = entry.step;
  if(exponential) {
    if(decreasing) {
      step = amplify(step, level);
    } else if(level > 0x6000) {
      if(rate < 40) {
        step >>= 2;
      } else if(rate >= 44) {
        increment >>= 2;
      } else {
        step >>= 1;
        increment >>= 1;
      }
    }
  }
  counter += max(1, increment);
  if(counter < 0x8000) return level;
  counter = 0;
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
    Envelope::reset(rate, decreasing, exponential);
    active = counter >= 0;
  }
}

auto SPU::VolumeSweep::tick() -> void {
  if(!active) return;
  bool subtract = exponential && decreasing ? true : bool(decreasing ^ negative);
  auto& entry = spu.adsrEntries[subtract][rate];
  s32 increment = entry.increment;
  s32 step = entry.step;
  if(exponential) {
    if(decreasing) {
      step = amplify(step, current);
    } else if(current > 0x6000) {
      if(rate < 40) step >>= 2;
      else if(rate >= 44) increment >>= 2;
      else { step >>= 1; increment >>= 1; }
    }
  }
  counter += max(1, increment);
  if(counter < 0x8000) return;
  counter = 0;
  s32 next = current + step;
  if(!decreasing) next = sclamp<16>(next);
  else if(negative) next = min(0, max(-0x8000, next));
  else next = max(0, next);
  current = next;
  active = decreasing ? current != 0 : current != (negative ? -0x8000 : 0x7fff);
}
