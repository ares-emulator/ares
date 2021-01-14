auto CPU::serialize(serializer& s) -> void {
  WDC65816::serialize(s);
  Thread::serialize(s);
  PPUcounter::serialize(s);

  s(wram);

  s(counter.cpu);
  s(counter.dma);

  s(status.clockCount);

  s(status.irqLock);

  s(status.dramRefreshPosition);
  s(status.dramRefresh);

  s(status.hdmaSetupPosition);
  s(status.hdmaSetupTriggered);

  s(status.hdmaPosition);
  s(status.hdmaTriggered);

  s(status.nmiValid);
  s(status.nmiLine);
  s(status.nmiTransition);
  s(status.nmiPending);
  s(status.nmiHold);

  s(status.irqValid);
  s(status.irqLine);
  s(status.irqTransition);
  s(status.irqPending);
  s(status.irqHold);

  s(status.resetPending);
  s(status.interruptPending);

  s(status.dmaActive);
  s(status.dmaPending);
  s(status.hdmaPending);
  s(status.hdmaMode);

  s(status.autoJoypadCounter);

  s(io.wramAddress);

  s(io.hirqEnable);
  s(io.virqEnable);
  s(io.irqEnable);
  s(io.nmiEnable);
  s(io.autoJoypadPoll);

  s(io.pio);

  s(io.wrmpya);
  s(io.wrmpyb);

  s(io.wrdiva);
  s(io.wrdivb);

  s(io.htime);
  s(io.vtime);

  s(io.romSpeed);

  s(io.version);

  s(io.rddiv);
  s(io.rdmpy);

  s(io.joy1);
  s(io.joy2);
  s(io.joy3);
  s(io.joy4);

  s(alu.mpyctr);
  s(alu.divctr);
  s(alu.shift);

  for(auto& channel : channels) {
    s(channel.dmaEnable);
    s(channel.hdmaEnable);
    s(channel.direction);
    s(channel.indirect);
    s(channel.unused);
    s(channel.reverseTransfer);
    s(channel.fixedTransfer);
    s(channel.transferMode);
    s(channel.targetAddress);
    s(channel.sourceAddress);
    s(channel.sourceBank);
    s(channel.transferSize);
    s(channel.indirectBank);
    s(channel.hdmaAddress);
    s(channel.lineCounter);
    s(channel.unknown);
    s(channel.hdmaCompleted);
    s(channel.hdmaDoTransfer);
  }
}
