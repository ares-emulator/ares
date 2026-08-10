auto ANTIC::Playfield::power() -> void {
  dmaClock = 0;
  for(auto& name : dmaName) name = 0;
  lineWrite = 0;
  lineRead = 0;
  for(auto& data : lineBuffer) data = 0;
  shiftClock = 0;
  graphics = 0;
  name = 0;
  output = 0;
  delayed = 0;
  queuePosition = 0;
  queueValid = 0;
  queueLine = 0;
  for(auto& data : queueData) data = 0;
  for(auto& name : queueName) name = 0;
  for(auto& phase : queuePhase) phase = 0;
}

auto ANTIC::Playfield::start() const -> u16 {
  switch((u8)self.io.dmactl & 3) {
  case 1: return 0x40 * Timing::SamplesPerColorClock;
  case 2: return 0x30 * Timing::SamplesPerColorClock;
  case 3: return 0x2c * Timing::SamplesPerColorClock;
  }
  return 0;
}

auto ANTIC::Playfield::end() const -> u16 {
  switch((u8)self.io.dmactl & 3) {
  case 1: return 0xc0 * Timing::SamplesPerColorClock;
  case 2: return 0xd0 * Timing::SamplesPerColorClock;
  case 3: return 0xe0 * Timing::SamplesPerColorClock;
  }
  return 0;
}

auto ANTIC::Playfield::fetchWidth() const -> u8 {
  auto width = (u8)self.io.dmactl & 3;
  if(self.displayList.instruction.bit(4) && width && width < 3) width++;
  return width;
}

auto ANTIC::Playfield::sample() const -> n3 {
  auto mode = (u8)self.displayList.instruction & 15;
  auto value = (u8)output;
  if((mode == 2 || mode == 3) && name.bit(7)) {
    if(self.io.chactl.bit(0)) value = 0;
    if(self.io.chactl.bit(1)) value ^= 3;
  }
  if((mode == 4 || mode == 5) && value == 3 && name.bit(7)) return 4;
  if(mode == 6 || mode == 7) return value ? (((u8)name >> 6) + 1) : 0;
  return value;
}

auto ANTIC::Playfield::clockAN(u16 colorClock) -> n3 {
  clockShiftRing();

  // ANTIC machine cycle 0 precedes GTIA horizontal position 0 by three
  // color clocks. Translate the machine-cycle phase into the coordinate that
  // GTIA is sampling before selecting the playfield data for the AN bus.
  colorClock = (colorClock + Timing::ColorClocksPerScanline - 3)
    % Timing::ColorClocksPerScanline;

  auto mode = (u8)self.displayList.instruction & 15;
  if(self.counter.scanline < Timing::DisplayListFirstScanline
  || self.counter.scanline > Timing::DisplayListLastScanline) {
    auto blank = self.counter.scanline >= 251 && self.counter.scanline <= 253
      ? 1 : Modes[mode].highResolution ? 3 : 2;
    auto sample = colorClock * Timing::SamplesPerColorClock;
    bool corruptHiresBlank = Modes[mode].highResolution
      && (self.io.dmactl & 3)
      && sample >= start()
      && sample < end();
    return corruptHiresBlank ? blank | 4 : blank;
  }
  if(colorClock < Timing::VisibleFirstColorClock || colorClock > Timing::VisibleLastColorClock) {
    return Modes[mode].highResolution ? 3 : 2;
  }

  auto position = colorClock * Timing::SamplesPerColorClock;
  if(!self.displayList.valid || mode < 2 || !(self.io.dmactl & 3)) {
    delayed = 0;
    return 0;
  }

  auto value = (u8)sample();
  n3 encoded = Modes[mode].highResolution ? n3{4 | value} : n3{value ? value + 3 : 0};
  auto an = self.displayList.instruction.bit(4) && self.io.hscroll.bit(0) ? delayed : encoded;
  delayed = encoded;
  if(position < start() || position >= end()) return 0;
  return an;
}

auto ANTIC::Playfield::characterAddress(n8 name, u8 row) const -> n16 {
  auto mode = (u8)self.displayList.instruction & 15;
  auto character = mode < 6 ? (u8)name & 0x7f : (u8)name & 0x3f;
  auto characterRow = mode == 5 || mode == 7 ? (row >> 1) & 7 : row & 7;

  if(mode == 3) {
    if(character < 0x60 && row >= 8) return 0xffff;
    if(character >= 0x60) {
      if(row < 2) return 0xffff;
      characterRow = row & 7;
    }
  }
  if(self.io.chactl.bit(2)) characterRow ^= 7;

  auto mask = mode < 6 ? 0xfc00 : 0xfe00;
  return ((u16)self.io.chbasePipeline[1] << 8 & mask) | ((u16)character << 3) | characterRow;
}

auto ANTIC::Playfield::lineBufferRead(n6 phase) const -> n8 {
  if(phase < 48) return lineBuffer[phase];
  return 0xff;
}

auto ANTIC::Playfield::lineBufferWrite(n6 phase, n8 data) -> void {
  if(phase < 48) lineBuffer[phase] = data;
}

auto ANTIC::Playfield::bitmapLineBufferPhase(bool next) -> n6 {
  // Consecutive bitmap reads clock the address before every read except the
  // last one. A run [i..i+n-1] therefore returns
  // [i+1..i+n-1, i+n-1] while still advancing by n positions overall.
  if(next) incrementLineBuffer(lineRead);
  auto phase = lineRead;
  if(!next) incrementLineBuffer(lineRead);
  return phase;
}

auto ANTIC::Playfield::incrementLineBuffer(n6& phase) -> void {
  phase = phase == 62 ? 0 : phase + 1;
}

auto ANTIC::Playfield::dmaFeedbackLength() const -> u8 {
  auto mode = (u8)self.displayList.instruction & 15;
  if(mode >= 8 && mode <= 9) return 8;
  if(mode >= 6 && mode <= 7) return 4;
  if(mode >= 10 && mode <= 12) return 4;
  return 2;
}

auto ANTIC::Playfield::dmaStart() const -> u8 {
  auto mode = (u8)self.displayList.instruction & 15;
  auto width = fetchWidth();
  if(mode < 2 || !width) return 0xff;

  auto cycle = width == 1 ? 26 : width == 2 ? 18 : 10;
  if(self.displayList.instruction.bit(4)) cycle += (u8)self.io.hscroll >> 1;
  return cycle;
}

auto ANTIC::Playfield::dmaStop() const -> u8 {
  auto mode = (u8)self.displayList.instruction & 15;
  auto width = fetchWidth();
  if(mode < 2 || !width) return 0xff;

  auto cycle = width == 1 ? 90 : width == 2 ? 98 : 106;
  if(self.displayList.instruction.bit(4)) cycle += (u8)self.io.hscroll >> 1;
  return cycle;
}

auto ANTIC::Playfield::clearDMARing() -> void {
  dmaClock = 0;
  for(auto& name : dmaName) name = 0;
}

auto ANTIC::Playfield::clockDMARing() -> void {
  auto start = dmaStart();
  auto stop = dmaStop();

  if(self.counter.machineCycle == start) {
    dmaClock.bit(0) = 1;
    lineWrite = 0;
    lineRead = 0;
  }
  if(self.counter.machineCycle == stop) {
    dmaClock.bit(0) = 0;
    dmaName[0] = 0;
  }
}

auto ANTIC::Playfield::scheduleGraphics(n8 data, n8 name, u8 delay) -> void {
  auto slot = ((u8)queuePosition + delay) & 15;
  queueData[slot] |= data;
  queueName[slot] = name;
  queueValid.bit(slot) = 1;
}

auto ANTIC::Playfield::scheduleLineGraphics(n6 phase, u8 delay) -> void {
  auto slot = ((u8)queuePosition + delay) & 15;
  queueLine.bit(slot) = 1;
  queuePhase[slot] = phase;
  queueName[slot] = 0;
  queueValid.bit(slot) = 1;
}

auto ANTIC::Playfield::clockShiftRing() -> void {
  auto slot = (u8)queuePosition;
  if(self.counter.machineCycle < 8) {
    shiftClock = 0;
    graphics = 0;
    output = 0;
    delayed = 0;
    queueValid = 0;
    queueLine = 0;
    for(auto& data : queueData) data = 0;
  } else if(queueValid.bit(slot)) {
    if(queueLine.bit(slot)) {
      queueData[slot] |= lineBufferRead(queuePhase[slot]);
    }
    graphics |= queueData[slot];
    name = queueName[slot];
    shiftClock.bit(0) = 1;
    queueValid.bit(slot) = 0;
    queueLine.bit(slot) = 0;
    queueData[slot] = 0;
  }

  auto mode = (u8)self.displayList.instruction & 15;
  u8 shiftMask = 0x0f;
  u8 shiftBits = 1;
  switch(mode) {
  case 2: case 3: case 4: case 5:
    shiftBits = 2;
    break;
  case 8:
    shiftMask = 0x01;
    shiftBits = 2;
    break;
  case 9:
    shiftMask = 0x05;
    break;
  case 10:
    shiftMask = 0x05;
    shiftBits = 2;
    break;
  case 13: case 14: case 15:
    shiftBits = 2;
    break;
  }

  if((u8)shiftClock & shiftMask) {
    if(shiftBits == 2) {
      output = (u8)graphics >> 6;
      graphics <<= 2;
    } else {
      output = (u8)graphics >> 7;
      graphics <<= 1;
    }
  }

  shiftClock = ((u8)shiftClock << 1 | (u8)shiftClock >> 3) & 15;
  queuePosition = (slot + 1) & 15;
}

auto ANTIC::Playfield::queueDMA(DMA::Requests& requests) -> void {
  bool displayRange =
    self.counter.scanline >= Timing::DisplayListFirstScanline
    && self.counter.scanline <= Timing::DisplayListLastScanline;
  auto mode = (u8)self.displayList.instruction & 15;
  if(!displayRange || !self.displayList.valid || mode < 2) return;

  clockDMARing();
  bool enabled = (self.io.dmactl & 3) != 0;
  auto physical = enabled && self.counter.machineCycle >= 10 && self.counter.machineCycle <= 105;
  auto& properties = Modes[mode];

  if(properties.character && dmaClock.bit(0)) {
    auto readPhase = lineRead;
    auto name = lineBufferRead(readPhase);
    if(self.displayList.firstScanline) {
      auto writePhase = lineWrite;
      requests.append(
        self.displayList.memoryScan,
        DMAConsumer::LineBufferName, writePhase, true, physical, false, readPhase
      );
      self.displayList.memoryScan = self.displayList.incrementMemoryScan(self.displayList.memoryScan);
      incrementLineBuffer(lineWrite);
    } else {
      dmaName[0] = name;
    }
    incrementLineBuffer(lineRead);
  }

  if(!properties.character && dmaClock.bit(2)) {
    bool next = dmaClock.bit(1);
    auto readPhase = bitmapLineBufferPhase(next);
    if(self.displayList.firstScanline) {
      auto writePhase = lineWrite;
      requests.append(
        self.displayList.memoryScan,
        DMAConsumer::LineBufferBitmap, writePhase, true, physical, next, readPhase
      );
      self.displayList.memoryScan = self.displayList.incrementMemoryScan(self.displayList.memoryScan);
      incrementLineBuffer(lineWrite);
    } else {
      requests.replayLineBuffer(readPhase);
    }
  }

  if(properties.character && dmaClock.bit(3)) {
    auto name = dmaName[3];
    auto address = characterAddress(name, self.displayList.row);
    if(address == 0xffff) {
      scheduleGraphics(0, name, 7);
    } else {
      requests.append(
        address,
        DMAConsumer::Character, name, true, physical
      );
    }
  }

  auto length = dmaFeedbackLength();
  bool feedback = dmaClock.bit(length - 1);
  auto feedbackName = dmaName[length - 1];
  dmaClock <<= 1;
  for(s32 index = 7; index >= 1; index--) {
    dmaName[index] = dmaName[index - 1];
  }
  dmaClock.bit(0) = feedback;
  dmaName[0] = feedback ? feedbackName : n8{0};
}

auto ANTIC::Playfield::consumeName(n6 writePhase, n6 readPhase, n8 data) -> void {
  lineBufferWrite(writePhase, data);
  // The DMA ring has already advanced while the bus cycle was in flight.
  // Place the returned name in stage 1 so the stage-3 character request
  // observes it three machine cycles after the stage-0 name request.
  dmaName[1] = lineBufferRead(readPhase);
}

auto ANTIC::Playfield::consumeBitmap(n6 writePhase, n6 readPhase, n8 data) -> void {
  lineBufferWrite(writePhase, data);
  scheduleLineGraphics(readPhase, 9);
}

auto ANTIC::Playfield::consumeCharacter(n8 character, n8 data) -> void {
  scheduleGraphics(data, character, 7);
}
