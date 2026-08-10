auto ANTIC::DisplayList::power() -> void {
  instruction = 0;
  memoryScan = 0;
  row = 0;
  lastRow = 0;
  valid = 0;
  needInstruction = 1;
  waitingForVerticalBlank = 0;
  loadMemoryScan = 0;
  verticalScroll = 0;
  verticalScrollEnding = 0;
  firstScanline = 0;
  dmaEnabled = 0;
  operand = 0;
  lineAddress = 0;
  vscrollStart = 0;
  vscrollDLI = 0;
  vscrollEnd = 0;
}

auto ANTIC::DisplayList::incrementDisplayList(n16 address) const -> n16 {
  return (address & 0xfc00) | ((address + 1) & 0x03ff);
}

auto ANTIC::DisplayList::incrementMemoryScan(n16 address) const -> n16 {
  return (address & 0xf000) | ((address + 1) & 0x0fff);
}

auto ANTIC::DisplayList::beginInstruction() -> void {
  auto previousVerticalScroll = (bool)verticalScroll;
  operand = 0;
  lineAddress = 0;
  firstScanline = 1;

  auto mode = (u8)instruction & 15;
  verticalScroll = mode >= 2 && instruction.bit(5);
  verticalScrollEnding = previousVerticalScroll && !verticalScroll;
  row = verticalScroll && !previousVerticalScroll ? (u8)vscrollStart : 0;
  if(mode == 0) {
    lastRow = ((u8)instruction >> 4 & 7);
  } else if(mode == 1) {
    lastRow = 0;
    operand = 2;
  } else {
    lastRow = Modes[mode].height - 1;
    loadMemoryScan = instruction.bit(6);
    operand = loadMemoryScan ? 2 : 0;
    lineAddress = memoryScan;
  }
  if(mode < 2) self.playfield.clearDMARing();
}

auto ANTIC::DisplayList::beginScanline() -> void {
  // AHRM 4.6: vertical blank preserves the retained IR except for LMS/JVB bit 6.
  if(self.counter.scanline == 248) {
    instruction &= ~0x40;
    self.playfield.clearDMARing();
  }
  if(self.counter.scanline == Timing::DisplayListFirstScanline && waitingForVerticalBlank) {
    waitingForVerticalBlank = 0;
    needInstruction = 1;
  }

  self.playfield.lineWrite = 0;
  self.playfield.lineRead = 0;
}

auto ANTIC::DisplayList::finishScanline() -> void {
  bool displayRange =
    self.counter.scanline >= Timing::DisplayListFirstScanline
    && self.counter.scanline <= Timing::DisplayListLastScanline;
  if(displayRange && valid) {
    if(waitingForVerticalBlank) {
      if(verticalScrollEnding && row != lastRow) {
        row = ((u8)row + 1) & 15;
        firstScanline = 0;
      } else {
        row = 0;
        lastRow = 0;
        verticalScrollEnding = 0;
        firstScanline = 1;
      }
    } else if(row == lastRow) {
      needInstruction = 1;
      row = 0;
    } else {
      row = ((u8)row + 1) & 15;
      firstScanline = 0;
    }
  }
}

auto ANTIC::DisplayList::clock() -> void {
  // AHRM 4.7: VSCROL has separate cycle 0, 5, and 108 deadlines for
  // entering a region, deciding its DLI, and ending the region.
  if(self.counter.machineCycle == 0) {
    vscrollStart = self.io.vscroll;
    auto mode = (u8)instruction & 15;
    if(!needInstruction && !waitingForVerticalBlank
    && verticalScrollEnding && mode == 1 && !instruction.bit(6) && row) {
      // AHRM 4.6: an extended ordinary JMP follows an indirect-address chain.
      // JVB fetches its target only on its first line and then replays without
      // any further display-list DMA.
      operand = 2;
      lineAddress = 0;
    }
  }

  if(self.counter.machineCycle == 5) {
    vscrollDLI = self.io.vscroll;
    bool displayRange =
      self.counter.scanline >= Timing::DisplayListFirstScanline
      && self.counter.scanline <= Timing::DisplayListLastScanline;
    if(displayRange && valid && !needInstruction && instruction.bit(7)) {
      auto last = verticalScrollEnding ? (u8)vscrollDLI : (u8)lastRow;
      if(row == last) self.interrupt.schedule(0x80);
    }
  }

  if(self.counter.machineCycle == 108) {
    vscrollEnd = self.io.vscroll;
    if(verticalScrollEnding) lastRow = vscrollEnd;
  }
}

auto ANTIC::DisplayList::queueDMA(DMA::Requests& requests) -> void {
  bool displayRange =
    self.counter.scanline >= Timing::DisplayListFirstScanline
    && self.counter.scanline <= Timing::DisplayListLastScanline;
  if(!displayRange || waitingForVerticalBlank) return;

  if(self.counter.machineCycle == 1 && needInstruction) {
    if(dmaEnabled) {
      requests.append(
        self.io.dlist, DMAConsumer::DisplayListInstruction, 0, true, true
      );
      return;
    }
    if(valid) {
      // The retained instruction is decoded even though its cycle-1 fetch was
      // suppressed. Address operands remain gated by the live DMACTL value.
      needInstruction = 0;
      beginInstruction();
    }
  }

  if(self.io.dmactl.bit(5) && operand
  && (self.counter.machineCycle == 6 || self.counter.machineCycle == 7)) {
    requests.append(
      self.io.dlist, DMAConsumer::DisplayListOperand, self.counter.machineCycle - 6, true, true
    );
  }
}

auto ANTIC::DisplayList::consumeInstruction(n8 data) -> void {
  instruction = data;
  if(!self.io.playerMissileDMA[1]) {
    // With GTIA missile DMA acceptance enabled but no ANTIC P/M slot,
    // the first HALT can expose the display-list byte on the shared bus.
    gtia.loadMissileDMA(data, self.counter.scanline);
  }
  self.io.dlist = incrementDisplayList(self.io.dlist);
  valid = 1;
  needInstruction = 0;
  beginInstruction();
}

auto ANTIC::DisplayList::consumeOperand(u8 index, n8 data) -> void {
  self.io.dlist = incrementDisplayList(self.io.dlist);
  lineAddress.byte(index) = data;
  operand--;
  if(operand) return;

  auto mode = (u8)instruction & 15;
  if(mode == 1) {
    self.io.dlist = lineAddress;
    if(instruction.bit(6)) waitingForVerticalBlank = 1;
  } else {
    memoryScan = lineAddress;
  }
}
