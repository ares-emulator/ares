auto SPU::adsrConstructTable() -> void {
  for(u8 decreasing : range(2)) {
    for(u8 rate : range(128)) {
      auto& entry = adsrEntries[decreasing][rate];
      if(rate < 48) {
        entry.ticks = 1;
        if(decreasing == 0) entry.step = +7 - (rate & 3) << 11 - (rate >> 2);
        if(decreasing == 1) entry.step = -8 + (rate & 3) << 11 - (rate >> 2);
      } else {
        entry.ticks = 1 << (rate >> 2) - 11;
        if(decreasing == 0) entry.step = +7 - (rate & 3);
        if(decreasing == 1) entry.step = -8 + (rate & 3);
      }
    }
  }
}
