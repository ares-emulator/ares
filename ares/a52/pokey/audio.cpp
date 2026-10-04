auto POKEY::Audio::power() -> void {
  for(auto& item : channel) {
    item = {};
    item.filterLatch = 1;
  }
  control = 0;
  timersRunning = 0;
  startDelay = 0;
  twoToneResyncPipeline = 0;
  reloadTimers();
  timersRunning = 0;
}

auto POKEY::Audio::clockFilters() -> void {
  for(u32 index = 0; index < 2; index++) {
    auto& item = channel[index];
    if(!item.filterDelay || --item.filterDelay) continue;
    item.filterLatch = item.filterSample;
  }
}

auto POKEY::Audio::clockTimers(const Clock::Pulses& pulses, const Clock& clock,
  bool holdTimer34) -> TimerEdges {
  auto pipelineEvents = advanceTimerPipelines(clock, holdTimer34);
  auto channelEvents = pipelineEvents.channels;
  auto reloadedChannels = pipelineEvents.reloads;
  bool timersReloaded = false;
  if(startDelay && !--startDelay) {
    reloadTimers();
    timersReloaded = true;
  }

  for(u32 index = 0; index < 4; index++) {
    if(holdTimer34 && index >= 2) continue;
    auto fastClock = channelUsesFastClock(index);
    auto channelClock = fastClock || (control.bit(0) ? pulses.clock15 : pulses.clock64);
    if(timersReloaded || (reloadedChannels & 1 << index) || !channelClock) continue;
    if(clockTimerCounter(index, clock)) channelEvents |= 1 << index;
  }

  advanceTwoToneResynchronization();
  return {
    .timer1 = (channelEvents & 1 << 0) != 0,
    .timer2 = (channelEvents & 1 << 1) != 0,
    .timer4 = (channelEvents & 1 << 3) != 0,
  };
}

auto POKEY::Audio::writeFrequency(u32 index, n8 data) -> void {
  channel[index].frequency = data;
}

auto POKEY::Audio::writeControl(u32 index, n8 data) -> void {
  channel[index].control = data;
}

auto POKEY::Audio::writeAUDCTL(n8 data) -> void {
  control = data;
  if(!control.bit(2)) {
    channel[0].filterLatch = 1;
    channel[0].filterDelay = 0;
  }
  if(!control.bit(1)) {
    channel[1].filterLatch = 1;
    channel[1].filterDelay = 0;
  }
}

auto POKEY::Audio::startTimers() -> void {
  timersRunning = 1;
  startDelay = 3;
}

auto POKEY::Audio::scheduleTwoToneResynchronization(u32 timerEvents) -> void {
  bool delayed = ((timerEvents & 1 << 0) && channelUsesFastClock(0))
    || ((timerEvents & 1 << 1) && channelUsesFastClock(1));
  if(!delayed) {
    resynchronizeTimer12();
    return;
  }
  twoToneResyncPipeline |= 1 << 1;
}

auto POKEY::Audio::channelPeriod(u32 index) const -> u32 {
  if(index == 0 && control.bit(6)) return (u8)channel[0].frequency + 4;
  if(index == 2 && control.bit(5)) return (u8)channel[2].frequency + 4;
  return (u8)channel[index].frequency + 1;
}

auto POKEY::Audio::channelIsJoined(u32 index) const -> bool {
  return control.bit(index < 2 ? 4 : 3);
}

auto POKEY::Audio::channelIsJoinedHigh(u32 index) const -> bool {
  return channelIsJoined(index) && (index & 1);
}

auto POKEY::Audio::channelUsesFastClock(u32 index) const -> bool {
  return (index == 0 && control.bit(6))
    || (index == 1 && control.bit(4) && control.bit(6))
    || (index == 2 && control.bit(5))
    || (index == 3 && control.bit(3) && control.bit(5));
}

auto POKEY::Audio::clockTimerCounter(u32 index, const Clock& clock) -> bool {
  if(channelIsJoinedHigh(index)) return false;

  auto& item = channel[index];
  auto fastClock = channelUsesFastClock(index);
  auto joined = channelIsJoined(index);
  if(fastClock && joined) {
    if(item.counter && --item.counter) return false;
    item.counter = 256;
    clockChannel(index, clock);
    return true;
  }

  if(item.eventDelay || item.reloadDelay) return false;
  if(item.counter && --item.counter) return false;

  if(fastClock) {
    item.eventDelay = 4;
    item.reloadDelay = 3;
    return false;
  }

  item.counter = joined ? 256 : 0;
  item.eventDelay = 5;
  if(!joined) item.reloadDelay = 3;
  return false;
}

auto POKEY::Audio::advanceTimerPipelines(const Clock& clock, bool holdTimer34) -> PipelineEvents {
  u32 events = 0;
  u32 reloads = 0;
  for(u32 index = 0; index < 4; index++) {
    if(holdTimer34 && index >= 2) continue;
    auto& item = channel[index];
    if(item.eventDelay && !--item.eventDelay) events |= 1 << index;
    if(item.reloadDelay && !--item.reloadDelay) reloads |= 1 << index;
  }
  for(u32 index = 0; index < 4; index++) {
    if(events & 1 << index) clockChannel(index, clock);
  }
  for(u32 index = 0; index < 4; index++) {
    if(!(reloads & 1 << index)) continue;
    channel[index].counter = (u8)channel[index].frequency + 1;
  }
  return {.channels = events, .reloads = reloads};
}

auto POKEY::Audio::advanceTwoToneResynchronization() -> void {
  if(twoToneResyncPipeline.bit(0)) resynchronizeTimer12();
  twoToneResyncPipeline >>= 1;
}

auto POKEY::Audio::resynchronizeTimer(u32 index) -> void {
  auto joined = channelIsJoined(index);
  if(channelUsesFastClock(index) && !joined) {
    channel[index].counter = (u8)channel[index].frequency + 1;
    channel[index].eventDelay = 4;
    channel[index].reloadDelay = 3;
    return;
  }
  channel[index].counter = channelPeriod(index);
  channel[index].eventDelay = 0;
  channel[index].reloadDelay = 0;
}

auto POKEY::Audio::resynchronizeTimer12() -> void {
  resynchronizeTimer(0);
  resynchronizeTimer(1);
}

auto POKEY::Audio::resetSerialTimer34() -> void {
  for(u32 index = 2; index < 4; index++) {
    channel[index].counter = channelIsJoinedHigh(index) ? (u8)channel[index].frequency + 1 : channelPeriod(index);
    channel[index].eventDelay = 0;
    channel[index].reloadDelay = 0;
  }
}

auto POKEY::Audio::clockChannel(u32 index, const Clock& clock) -> void {
  clockChannelSource(index, clock);
  sampleChannelFilters(index);
  requestChannelIRQ(index);
  clockJoinedCounters(index);
}

auto POKEY::Audio::clockChannelSource(u32 index, const Clock& clock) -> void {
  auto& item = channel[index];
  auto gate = item.control.bit(7) || clock.sample5(index);
  if(gate) {
    if(item.control.bit(5)) item.output ^= 1;
    else if(item.control.bit(6)) item.output = clock.sample4(index);
    else if(control.bit(7)) item.output = clock.sample9(index);
    else item.output = clock.sample17(index);
  }
}

auto POKEY::Audio::sampleChannelFilters(u32 index) -> void {
  if(index == 2 && control.bit(2)) {
    channel[0].filterSample = channel[0].output;
    channel[0].filterDelay = 2;
  }
  if(index == 3 && control.bit(1)) {
    channel[1].filterSample = channel[1].output;
    channel[1].filterDelay = 2;
  }
}

auto POKEY::Audio::requestChannelIRQ(u32 index) -> void {
  if(timersRunning) {
    if(index == 0 && !channelIsJoined(index)) self.irq.request(IRQ::Timer1);
    if(index == 1) self.irq.request(IRQ::Timer2);
    if(index == 3) self.irq.request(IRQ::Timer4);
  }
}

auto POKEY::Audio::clockJoinedCounters(u32 index) -> void {
  if(index == 0 && channelIsJoined(index)) {
    if(channel[1].counter && !--channel[1].counter) channel[1].eventDelay = 3;
  }
  if(index == 2 && channelIsJoined(index)) {
    if(channel[3].counter && !--channel[3].counter) channel[3].eventDelay = 3;
  }
  if(index == 1 && channelIsJoined(index)) {
    channel[0].counter = channelPeriod(0);
    channel[1].counter = (u8)channel[1].frequency + 1;
  }
  if(index == 3 && channelIsJoined(index)) {
    channel[2].counter = channelPeriod(2);
    channel[3].counter = (u8)channel[3].frequency + 1;
  }
}

auto POKEY::Audio::reloadTimers() -> void {
  for(u32 index = 0; index < 4; index++) {
    auto joined = channelIsJoined(index);
    auto joinedHigh = channelIsJoinedHigh(index);
    if(joinedHigh || (channelUsesFastClock(index) && !joined)) {
      channel[index].counter = (u8)channel[index].frequency + 1;
    } else {
      channel[index].counter = channelPeriod(index);
    }
    channel[index].eventDelay = 0;
    channel[index].reloadDelay = 0;
    channel[index].output = 1;
  }
}

auto POKEY::Audio::levels() const -> Levels {
  Levels levels;
  for(u32 index = 0; index < 4; index++) {
    auto& source = channel[index];
    auto waveform = (bool)source.output;
    if(index < 2) waveform ^= source.filterLatch;
    auto active = source.control.bit(4) || waveform;
    levels.channel[index] = active ? (u8)source.control & 15 : 0;
  }
  return levels;
}
