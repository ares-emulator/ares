auto PCD::serialize(serializer& s) -> void {
  Thread::serialize(s);
  wram.serialize(s);
  bram.serialize(s);
  if(Model::PCEngineDuo()) sram.serialize(s);
  drive.serialize(s);
  scsi.serialize(s);
  cdda.serialize(s);
  adpcm.serialize(s);
  fader.serialize(s);

  s.integer(clock.drive);
  s.integer(clock.cdda);
  s.integer(clock.adpcm);
  s.integer(clock.fader);

  s.array(io.mdr);
  s.integer(io.cddaSampleSelect);
  s.integer(io.sramEnable);
  s.integer(io.bramEnable);
}

auto PCD::Drive::serialize(serializer& s) -> void {
  s.integer((uint&)mode);
  s.integer((uint&)seek);
  s.integer(latency);
  s.integer(lba);
  s.integer(start);
  s.integer(end);
  s.array(sector);
}

auto PCD::SCSI::serialize(serializer& s) -> void {
  s.integer(irq.ready.enable);
  s.integer(irq.ready.line);
  s.integer(irq.completed.enable);
  s.integer(irq.completed.line);

  s.integer(pin.reset);
  s.integer(pin.acknowledge);
  s.integer(pin.select);
  s.integer(pin.input);
  s.integer(pin.control);
  s.integer(pin.message);
  s.integer(pin.request);
  s.integer(pin.busy);

  s.integer(bus.select);
  s.integer(bus.data);

  s.integer(acknowledging);
  s.integer(dataTransferCompleted);
  s.integer(messageAfterStatus);
  s.integer(messageSent);
  s.integer(statusSent);

  s.array(request.data);
  s.integer(request.reads);
  s.integer(request.writes);

  s.array(response.data);
  s.integer(response.reads);
  s.integer(response.writes);
}

auto PCD::CDDA::serialize(serializer& s) -> void {
  s.integer((uint&)playMode);
  s.integer(sample.left);
  s.integer(sample.right);
  s.integer(sample.offset);
}

auto PCD::ADPCM::serialize(serializer& s) -> void {
  msm5205.serialize(s);
  memory.serialize(s);

  s.integer(irq.halfReached.enable);
  s.integer(irq.halfReached.line);
  s.integer(irq.endReached.enable);
  s.integer(irq.endReached.line);

  s.integer(io.writeOffset);
  s.integer(io.writeLatch);
  s.integer(io.readOffset);
  s.integer(io.readLatch);
  s.integer(io.lengthLatch);
  s.integer(io.playing);
  s.integer(io.noRepeat);
  s.integer(io.reset);

  s.integer(read.address);
  s.integer(read.data);
  s.integer(read.pending);

  s.integer(write.address);
  s.integer(write.data);
  s.integer(write.pending);

  s.integer(sample.data);
  s.integer(sample.nibble);

  s.integer(dmaActive);
  s.integer(divider);
  s.integer(period);
  s.integer(latch);
  s.integer(length);
}

auto PCD::Fader::serialize(serializer& s) -> void {
  s.integer((uint&)mode);
  s.real(step);
  s.real(volume);
}
