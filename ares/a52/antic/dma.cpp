auto ANTIC::DMA::Requests::append(n16 address, DMAConsumer consumer, u8 index,
  bool addressValid, bool physical, bool next, u8 phase) -> void {
  assert(size < 4);
  item[size++] = {address, consumer, index, phase, addressValid, physical, next};
}

auto ANTIC::DMA::Requests::replayLineBuffer(n6 phase) -> void {
  lineBufferReplay = 1;
  lineBufferPhase = phase;
}

auto ANTIC::DMA::power() -> void {
  refreshPending = 0;
}

auto ANTIC::DMA::beginScanline() -> void {
  refreshPending = 0;
}

auto ANTIC::DMA::clockRefresh() -> void {
  if(self.counter.machineCycle >= 25 && self.counter.machineCycle <= 57
  && (self.counter.machineCycle - 25) % 4 == 0) {
    refreshPending = 1;
  }
}

auto ANTIC::DMA::playerMissileAddress(u8 index) const -> n16 {
  bool singleLine = self.io.dmactl.bit(4);
  auto row = singleLine ? (u16)self.counter.scanline : (u16)self.counter.scanline >> 1;
  auto base = (u16)self.io.pmbase << 8;
  if(singleLine) {
    base &= 0xf800;
    return base + (index ? 0x400 + (index - 1) * 0x100 : 0x300) + row;
  }
  base &= 0xfc00;
  return base + (index ? 0x200 + (index - 1) * 0x80 : 0x180) + row;
}

auto ANTIC::DMA::queuePlayerMissile(Requests& requests) -> void {
  bool displayRange =
    self.counter.scanline >= Timing::DisplayListFirstScanline
    && self.counter.scanline <= Timing::DisplayListLastScanline;
  auto enabled = self.io.playerMissileDMA[1];

  if(displayRange && self.counter.machineCycle == 0) {
    bool physical = enabled.bit(0) || enabled.bit(1);
    requests.append(
      physical ? playerMissileAddress(0) : n16{0},
      DMAConsumer::Missile, 0, physical, physical
    );
  }

  if(displayRange && self.counter.machineCycle >= 2 && self.counter.machineCycle <= 5) {
    auto player = (u8)self.counter.machineCycle - 2;
    bool physical = enabled.bit(1);
    requests.append(
      physical ? playerMissileAddress(player + 1) : n16{0},
      DMAConsumer::Player, player, physical, physical
    );
  }
}

auto ANTIC::DMA::arbitrate(Requests& requests) -> void {
  bool physical = false;
  n16 address = 0xffff;

  for(u32 index : range((u32)requests.size)) {
    auto& request = requests.item[index];
    if(request.addressValid) address &= request.address;
    physical |= request.physical;
  }

  n8 data;
  if(physical) {
    cpu.rdyLine(0);
    data = cpu.readDMA(address);
    self.step(Timing::ColorClocksPerMachineCycle);
    cpu.rdyLine(!self.wsync.active);
  } else if(refreshPending) {
    cpu.rdyLine(0);
    self.step(Timing::ColorClocksPerMachineCycle);
    cpu.rdyLine(!self.wsync.active);
    refreshPending = 0;
    data = cpu.dataBus();
  } else {
    self.step(Timing::ColorClocksPerMachineCycle);
    data = cpu.dataBus();
  }

  for(u32 index : range((u32)requests.size)) {
    consume(requests.item[index], data);
  }
  if(requests.lineBufferReplay) {
    self.playfield.scheduleLineGraphics(requests.lineBufferPhase, 9);
  }
}

auto ANTIC::DMA::consume(const Request& request, n8 data) -> void {
  switch(request.consumer) {
  case DMAConsumer::Missile:
    gtia.loadMissileDMA(data, self.counter.scanline);
    return;
  case DMAConsumer::Player:
    gtia.loadPlayerDMA(request.index, data, self.counter.scanline);
    return;
  case DMAConsumer::DisplayListInstruction:
    self.displayList.consumeInstruction(data);
    return;
  case DMAConsumer::DisplayListOperand:
    self.displayList.consumeOperand(request.index, data);
    return;
  case DMAConsumer::LineBufferName:
    self.playfield.consumeName(request.index, request.phase, data);
    return;
  case DMAConsumer::LineBufferBitmap:
    self.playfield.consumeBitmap(request.index, request.phase, data);
    return;
  case DMAConsumer::Character:
    self.playfield.consumeCharacter(request.index, data);
    return;
  }
}
