auto MCD::serialize(serializer& s) -> void {
  M68K::serialize(s);
  Thread::serialize(s);

  pram.serialize(s);
  wram.serialize(s);
  bram.serialize(s);

  s.integer(counter.divider);
  s.integer(counter.dma);
  s.real(counter.pcm);

  s.integer(io.run);
  s.integer(io.request);
  s.integer(io.halt);
  s.integer(io.wramLatch);
  s.integer(io.wramMode);
  s.integer(io.wramSwitch);
  s.integer(io.wramSelect);
  s.integer(io.wramPriority);
  s.integer(io.pramBank);
  s.integer(io.pramProtect);

  s.integer(led.red);
  s.integer(led.green);

  s.integer(irq.pending);
  irq.reset.serialize(s);
  irq.subcode.serialize(s);
  external.irq.serialize(s);

  s.integer(communication.cfm);
  s.integer(communication.cfs);
  s.array(communication.command);
  s.array(communication.status);

  cdc.serialize(s);
  cdd.serialize(s);
  timer.serialize(s);
  gpu.serialize(s);
  pcm.serialize(s);
}

auto MCD::IRQ::serialize(serializer& s) -> void {
  s.integer(enable);
  s.integer(pending);
}

auto MCD::CDC::serialize(serializer& s) -> void {
  ram.serialize(s);

  s.integer(address);
  s.integer(stopwatch);

  irq.decoder.serialize(s);
  irq.transfer.serialize(s);
  irq.command.serialize(s);

  s.array(command.fifo);
  s.integer(command.read);
  s.integer(command.write);
  s.integer(command.empty);

  s.array(status.fifo);
  s.integer(status.read);
  s.integer(status.write);
  s.integer(status.empty);
  s.integer(status.enable);
  s.integer(status.active);
  s.integer(status.busy);
  s.integer(status.wait);

  transfer.serialize(s);

  s.integer(decoder.enable);
  s.integer(decoder.mode);
  s.integer(decoder.form);
  s.integer(decoder.valid);

  s.integer(header.minute);
  s.integer(header.second);
  s.integer(header.frame);
  s.integer(header.mode);

  s.integer(subheader.file);
  s.integer(subheader.channel);
  s.integer(subheader.submode);
  s.integer(subheader.coding);

  s.integer(control.head);
  s.integer(control.mode);
  s.integer(control.form);
  s.integer(control.commandBreak);
  s.integer(control.modeByteCheck);
  s.integer(control.erasureRequest);
  s.integer(control.writeRequest);
  s.integer(control.pCodeCorrection);
  s.integer(control.qCodeCorrection);
  s.integer(control.autoCorrection);
  s.integer(control.errorCorrection);
  s.integer(control.edcCorrection);
  s.integer(control.correctionWrite);
  s.integer(control.descramble);
  s.integer(control.syncDetection);
  s.integer(control.syncInterrupt);
  s.integer(control.erasureCorrection);
  s.integer(control.statusTrigger);
  s.integer(control.statusControl);
}

auto MCD::CDC::Transfer::serialize(serializer& s) -> void {
  s.integer(destination);
  s.integer(address);
  s.integer(source);
  s.integer(target);
  s.integer(pointer);
  s.integer(length);
  s.integer(enable);
  s.integer(active);
  s.integer(busy);
  s.integer(wait);
  s.integer(ready);
  s.integer(completed);
}

auto MCD::CDD::serialize(serializer& s) -> void {
  irq.serialize(s);
  s.integer(counter);

  s.integer(dac.rate);
  s.integer(dac.deemphasis);
  s.integer(dac.attenuator);
  s.integer(dac.attenuated);
  dac.reconfigure();

  s.integer(io.status);
  s.integer(io.seeking);
  s.integer(io.latency);
  s.integer(io.sector);
  s.integer(io.sample);
  s.integer(io.track);

  s.integer(hostClockEnable);
  s.integer(statusPending);
  s.array(status);
  s.array(command);
}

auto MCD::Timer::serialize(serializer& s) -> void {
  irq.serialize(s);
  s.integer(counter);
}

auto MCD::GPU::serialize(serializer& s) -> void {
  irq.serialize(s);

  s.integer(font.color.background);
  s.integer(font.color.foreground);
  s.integer(font.data);

  s.integer(stamp.repeat);
  s.integer(stamp.tile.size);
  s.integer(stamp.map.size);
  s.integer(stamp.map.base);
  s.integer(stamp.map.address);

  s.integer(image.base);
  s.integer(image.offset);
  s.integer(image.vcells);
  s.integer(image.vdots);
  s.integer(image.hdots);
  s.integer(image.address);

  s.integer(vector.base);
  s.integer(vector.address);

  s.integer(active);
  s.integer(counter);
  s.integer(period);
}

auto MCD::PCM::serialize(serializer& s) -> void {
  ram.serialize(s);

  s.integer(io.enable);
  s.integer(io.bank);
  s.integer(io.channel);

  for(auto& channel : channels) channel.serialize(s);
}

auto MCD::PCM::Channel::serialize(serializer& s) -> void {
  s.integer(enable);
  s.integer(envelope);
  s.integer(pan);
  s.integer(step);
  s.integer(loop);
  s.integer(start);
  s.integer(address);
}
