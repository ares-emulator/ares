auto MCD::serialize(serializer& s) -> void {
  M68000::serialize(s);
  Thread::serialize(s);

  s(pram);
  s(wram);
  s(bram);

  s(counter.divider);
  s(counter.dma);
  s(counter.pcm);

  s(io.run);
  s(io.request);
  s(io.halt);
  s(io.wramLatch);
  s(io.wramMode);
  s(io.wramSwitchRequest);
  s(io.wramSwitch);
  s(io.wramSelect);
  s(io.wramPriority);
  s(io.pramBank);
  s(io.pramProtect);
  s(io.vectorLevel4);

  s(led.red);
  s(led.green);

  s(irq.pending);
  s(irq.reset);
  s(irq.subcode);
  s(external.irq);

  s(communication.cfm);
  s(communication.cfs);
  s(communication.command);
  s(communication.status);

  s(cdc);
  s(cdd);
  s(timer);
  s(gpu);
  s(pcm);
}

auto MCD::IRQ::serialize(serializer& s) -> void {
  s(enable);
  s(pending);
}

auto MCD::CDC::serialize(serializer& s) -> void {
  s(ram);

  s(address);
  s(stopwatch);

  s(irq.decoder);
  s(irq.transfer);
  s(irq.command);

  s(command.fifo);
  s(command.read);
  s(command.write);
  s(command.empty);

  s(status.fifo);
  s(status.read);
  s(status.write);
  s(status.empty);
  s(status.enable);
  s(status.active);
  s(status.busy);
  s(status.wait);

  s(transfer);

  s(decoder.enable);
  s(decoder.mode);
  s(decoder.form);
  s(decoder.valid);

  s(header.minute);
  s(header.second);
  s(header.frame);
  s(header.mode);

  s(subheader.file);
  s(subheader.channel);
  s(subheader.submode);
  s(subheader.coding);

  s(control.head);
  s(control.mode);
  s(control.form);
  s(control.commandBreak);
  s(control.modeByteCheck);
  s(control.erasureRequest);
  s(control.writeRequest);
  s(control.pCodeCorrection);
  s(control.qCodeCorrection);
  s(control.autoCorrection);
  s(control.errorCorrection);
  s(control.edcCorrection);
  s(control.correctionWrite);
  s(control.descramble);
  s(control.syncDetection);
  s(control.syncInterrupt);
  s(control.erasureCorrection);
  s(control.statusTrigger);
  s(control.statusControl);
}

auto MCD::CDC::Transfer::serialize(serializer& s) -> void {
  s(destination);
  s(address);
  s(source);
  s(target);
  s(pointer);
  s(length);
  s(enable);
  s(active);
  s(busy);
  s(wait);
  s(ready);
  s(completed);
}

auto MCD::CDD::serialize(serializer& s) -> void {
  s(irq);
  s(counter);

  s(dac.rate);
  s(dac.deemphasis);
  s(dac.attenuator);
  s(dac.attenuated);
  dac.reconfigure();

  s(io.status);
  s(io.seeking);
  s(io.latency);
  s(io.sector);
  s(io.sample);
  s(io.track);

  s(hostClockEnable);
  s(statusPending);
  s(status);
  s(command);
}

auto MCD::Timer::serialize(serializer& s) -> void {
  s(irq);
  s(frequency);
  s(counter);
}

auto MCD::GPU::serialize(serializer& s) -> void {
  s(irq);

  s(font.color.background);
  s(font.color.foreground);
  s(font.data);

  s(stamp.repeat);
  s(stamp.tile.size);
  s(stamp.map.size);
  s(stamp.map.base);
  s(stamp.map.address);

  s(image.base);
  s(image.offset);
  s(image.vcells);
  s(image.vdots);
  s(image.hdots);
  s(image.address);

  s(vector.base);
  s(vector.address);

  s(active);
  s(counter);
  s(period);
}

auto MCD::PCM::serialize(serializer& s) -> void {
  s(ram);

  s(io.enable);
  s(io.bank);
  s(io.channel);

  for(auto& channel : channels) s(channel);
}

auto MCD::PCM::Channel::serialize(serializer& s) -> void {
  s(enable);
  s(envelope);
  s(pan);
  s(step);
  s(loop);
  s(start);
  s(address);
}
