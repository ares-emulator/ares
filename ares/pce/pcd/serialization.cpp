auto PCD::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(wram);
  s(bram);
  if(Model::PCEngineDuo()) s(sram);
  s(drive);
  s(scsi);
  s(cdda);
  s(adpcm);
  s(fader);

  s(clock.drive);
  s(clock.cdda);
  s(clock.adpcm);
  s(clock.fader);

  s(io.mdr);
  s(io.cddaSampleSelect);
  s(io.sramEnable);
  s(io.bramEnable);
}

auto PCD::Drive::serialize(serializer& s) -> void {
  s((u32&)mode);
  s((u32&)seek);
  s(latency);
  s(lba);
  s(start);
  s(end);
  s(sector);
}

auto PCD::SCSI::serialize(serializer& s) -> void {
  s(irq.ready.enable);
  s(irq.ready.line);
  s(irq.completed.enable);
  s(irq.completed.line);

  s(pin.reset);
  s(pin.acknowledge);
  s(pin.select);
  s(pin.input);
  s(pin.control);
  s(pin.message);
  s(pin.request);
  s(pin.busy);

  s(bus.select);
  s(bus.data);

  s(acknowledging);
  s(dataTransferCompleted);
  s(messageAfterStatus);
  s(messageSent);
  s(statusSent);

  s(request.data);
  s(request.reads);
  s(request.writes);

  s(response.data);
  s(response.reads);
  s(response.writes);
}

auto PCD::CDDA::serialize(serializer& s) -> void {
  s((u32&)playMode);
  s(sample.left);
  s(sample.right);
  s(sample.offset);
}

auto PCD::ADPCM::serialize(serializer& s) -> void {
  s(msm5205);
  s(memory);

  s(irq.halfReached.enable);
  s(irq.halfReached.line);
  s(irq.endReached.enable);
  s(irq.endReached.line);

  s(io.writeOffset);
  s(io.writeLatch);
  s(io.readOffset);
  s(io.readLatch);
  s(io.lengthLatch);
  s(io.playing);
  s(io.noRepeat);
  s(io.reset);

  s(read.address);
  s(read.data);
  s(read.pending);

  s(write.address);
  s(write.data);
  s(write.pending);

  s(sample.data);
  s(sample.nibble);

  s(dmaActive);
  s(divider);
  s(period);
  s(latch);
  s(length);
}

auto PCD::Fader::serialize(serializer& s) -> void {
  s((u32&)mode);
  s(step);
  s(volume);
}
