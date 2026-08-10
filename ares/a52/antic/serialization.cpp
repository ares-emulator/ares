auto ANTIC::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(io.dmactl);
  s(io.chactl);
  s(io.dlist);
  s(io.hscroll);
  s(io.vscroll);
  s(io.pmbase);
  s(io.chbase);
  s(io.penh);
  s(io.penv);
  s(io.nmien);
  s(io.nmist);
  for(auto& chbase : io.chbasePipeline) s(chbase);
  for(auto& dma : io.playerMissileDMA) s(dma);

  s(counter.machineCycle);
  s(counter.scanline);

  displayList.serialize(s);
  dma.serialize(s);
  playfield.serialize(s);
  wsync.serialize(s);
  interrupt.serialize(s);
}

auto ANTIC::DisplayList::serialize(serializer& s) -> void {
  s(instruction);
  s(memoryScan);
  s(row);
  s(lastRow);
  s(valid);
  s(needInstruction);
  s(waitingForVerticalBlank);
  s(loadMemoryScan);
  s(verticalScroll);
  s(verticalScrollEnding);
  s(firstScanline);
  s(dmaEnabled);
  s(operand);
  s(lineAddress);
  s(vscrollStart);
  s(vscrollDLI);
  s(vscrollEnd);
}

auto ANTIC::DMA::serialize(serializer& s) -> void {
  s(refreshPending);
}

auto ANTIC::Playfield::serialize(serializer& s) -> void {
  s(dmaClock);
  for(auto& name : dmaName) s(name);
  s(lineWrite);
  s(lineRead);
  for(auto& data : lineBuffer) s(data);
  s(shiftClock);
  s(graphics);
  s(name);
  s(output);
  s(delayed);
  s(queuePosition);
  s(queueValid);
  s(queueLine);
  for(auto& data : queueData) s(data);
  for(auto& name : queueName) s(name);
  for(auto& phase : queuePhase) s(phase);
}

auto ANTIC::WSYNC::serialize(serializer& s) -> void {
  s(active);
  s(scanline);
}

auto ANTIC::Interrupt::serialize(serializer& s) -> void {
  s(pending);
  s(enable);
  s(pulse);
}
