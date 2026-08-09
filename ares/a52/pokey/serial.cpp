auto POKEY::Serial::power() -> void {
  inputValue = 0xff;
  inputShifter = 0;
  inputProgress = 0;
  outputHoldingValue = 0;
  outputHoldingFull = 0;
  outputShifter = 0;
  outputProgress = 0;
  inputClockPhase = 0;
  outputClockPhase = 0;
  outputLine = 1;
  twoToneOutput = 0;
  asynchronousReceiving = 0;
  previousInputLine = 1;
  previousExternalClock = 1;
  self.status.setSerialInput(true);
  self.status.setSerialBusy(false);
  self.status.setSerialOutput(true);
  self.irq.request(IRQ::SerialComplete);
}

auto POKEY::Serial::write(n8 data) -> void {
  if(!(self.control & 3)) return;
  outputHoldingValue = data;
  outputHoldingFull = 1;
}

auto POKEY::Serial::configure() -> void {
  auto serialMode = mode();
  if(serialMode == 0) {
    inputClockPhase = 0;
    outputClockPhase = 0;
    previousExternalClock = self.serialInput.clock;
  }
  if(!self.control.bit(3)) twoToneOutput = 0;
  if(self.control.bit(4) && !inputProgress) {
    asynchronousReceiving = 0;
    self.audio.resetSerialTimer34();
  } else if(self.control.bit(4)) {
    asynchronousReceiving = 1;
  }
  if(!self.control.bit(4)) asynchronousReceiving = 0;
  self.status.setSerialOutput(currentOutput());
}

auto POKEY::Serial::clock(const Audio::TimerEdges& timerEdges, SerialInput lines) -> void {
  self.status.setSerialInput(lines.data);
  auto edges = observeLines(lines);
  clockInputPath(timerEdges, lines.data, edges);
  clockOutputPath(timerEdges, edges);
  clockTwoTone(timerEdges);
  self.status.setSerialOutput(currentOutput());
}

auto POKEY::Serial::observeLines(SerialInput lines) -> LineEdges {
  LineEdges edges{
    .inputFalling = previousInputLine && !lines.data,
    .clockFalling = previousExternalClock && !lines.clock,
    .clockRising = !previousExternalClock && lines.clock,
  };
  previousInputLine = lines.data;
  previousExternalClock = lines.clock;
  return edges;
}

auto POKEY::Serial::clockInputPath(const Audio::TimerEdges& timerEdges, bool data,
  const LineEdges& edges) -> void {
  auto serialMode = mode();
  auto asynchronousInput = self.control.bit(4);
  if(asynchronousInput && !inputProgress && edges.inputFalling) {
    asynchronousReceiving = 1;
    inputClockPhase = 1;
    self.audio.resetSerialTimer34();
  }

  bool inputEdge = false;
  if(serialMode == 0 || serialMode == 4) {
    inputEdge = edges.clockFalling;
  } else if(timerEdges.timer4 && (!asynchronousInput || asynchronousReceiving)) {
    inputClockPhase ^= 1;
    inputEdge = !inputClockPhase;
  }
  if(inputEdge) {
    clockInput(data);
    if(asynchronousInput && !inputProgress) {
      asynchronousReceiving = 0;
      self.audio.resetSerialTimer34();
    }
  }
}

auto POKEY::Serial::clockOutputPath(const Audio::TimerEdges& timerEdges, const LineEdges& edges) -> void {
  auto serialMode = mode();
  bool outputTimerEdge = false;
  if(serialMode >= 2 && serialMode <= 5) outputTimerEdge = timerEdges.timer4;
  if(serialMode >= 6) outputTimerEdge = timerEdges.timer2;
  bool outputEdge = serialMode <= 1 ? edges.clockRising : false;
  if(serialMode <= 1 && edges.clockFalling) outputClockPhase = 0;
  if(serialMode <= 1 && edges.clockRising) outputClockPhase = 1;
  if(outputTimerEdge) {
    outputClockPhase ^= 1;
    outputEdge = outputClockPhase;
  }
  if(outputEdge) clockOutput();
}

auto POKEY::Serial::clockTwoTone(const Audio::TimerEdges& timerEdges) -> void {
  if(self.control.bit(3)) {
    u32 acceptedEvents = timerEdges.timer2 ? 1 << 1 : 0;
    if(timerEdges.timer1 && outputLine && !self.control.bit(7)) acceptedEvents |= 1 << 0;
    if(acceptedEvents) {
      twoToneOutput ^= 1;
      self.audio.scheduleTwoToneResynchronization(acceptedEvents);
    }
  }
}

auto POKEY::Serial::reset() -> void {
  inputShifter = 0;
  inputProgress = 0;
  outputHoldingValue = 0;
  outputHoldingFull = 0;
  outputShifter = 0;
  outputProgress = 0;
  inputClockPhase = 0;
  outputClockPhase = 0;
  asynchronousReceiving = 0;
  self.status.setSerialBusy(false);
  self.status.setSerialOutput(currentOutput());
  self.irq.request(IRQ::SerialComplete);
}

auto POKEY::Serial::input() const -> n8 {
  return inputValue;
}

auto POKEY::Serial::holdsTimer34() const -> bool {
  return self.control.bit(4) && !asynchronousReceiving;
}

auto POKEY::Serial::clockInput(bool data) -> void {
  if(!inputProgress) {
    if(data) return;
    inputShifter = 0;
    inputProgress = 1;
    self.status.setSerialBusy(true);
    return;
  }

  inputShifter.bit(inputProgress) = data;
  inputProgress++;
  if(inputProgress < 10) return;

  inputValue = inputShifter >> 1;
  if(!data) self.status.setSerialFrameError();
  if(self.irq.pending(IRQ::SerialInput)) self.status.setSerialInputOverrun();
  self.irq.request(IRQ::SerialInput);
  inputShifter = 0;
  inputProgress = 0;
  asynchronousReceiving = 0;
  self.status.setSerialBusy(false);
  if(self.control.bit(4)) self.audio.resetSerialTimer34();
}

auto POKEY::Serial::clockOutput() -> void {
  if(!outputProgress) {
    if(outputHoldingFull) loadOutput();
    return;
  }

  if(outputProgress < 10) {
    outputLine = outputShifter.bit(outputProgress);
    outputProgress++;
    return;
  }

  outputShifter = 0;
  outputProgress = 0;
  if(outputHoldingFull) loadOutput();
  else self.irq.request(IRQ::SerialComplete);
}

auto POKEY::Serial::loadOutput() -> void {
  outputShifter = (u16)outputHoldingValue << 1 | 1 << 9;
  outputProgress = 1;
  outputHoldingValue = 0;
  outputHoldingFull = 0;
  outputLine = outputShifter.bit(0);
  self.irq.acknowledge(IRQ::SerialComplete);
  self.irq.request(IRQ::SerialOutput);
}

auto POKEY::Serial::currentOutput() const -> bool {
  if(self.control.bit(3)) return twoToneOutput;
  if(self.control.bit(7)) return false;
  return outputLine;
}

auto POKEY::Serial::mode() const -> n3 {
  return (u8)self.control >> 4;
}
