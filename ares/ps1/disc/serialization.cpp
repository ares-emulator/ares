auto Disc::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(drive.lba.current);
  s(drive.lba.request);
  s(drive.lba.seeking);
  s(drive.sector.data);
  s(drive.sector.offset);
  s(drive.mode.cdda);
  s(drive.mode.autoPause);
  s(drive.mode.report);
  s(drive.mode.xaFilter);
  s(drive.mode.ignore);
  s(drive.mode.sectorSize);
  s(drive.mode.xaADPCM);
  s(drive.mode.speed);
  s(drive.seeking);

  s(audio.mute);
  s(audio.muteADPCM);
  s(audio.volume);
  s(audio.volumeLatch);

  s((u32&)cdda.playMode);
  s(cdda.sample.left);
  s(cdda.sample.right);

  s(cdxa.filter.file);
  s(cdxa.filter.channel);
  s(cdxa.sample.left);
  s(cdxa.sample.right);
  s(cdxa.monaural);
  s(cdxa.samples);
  s(cdxa.previousSamples);

  s(event.command);
  s(event.counter);
  s(event.invocation);
  s(event.queued);

  s(irq.flag);
  s(irq.mask);
  s(irq.ready.enable);
  s(irq.ready.flag);
  s(irq.complete.enable);
  s(irq.complete.flag);
  s(irq.acknowledge.enable);
  s(irq.acknowledge.flag);
  s(irq.end.enable);
  s(irq.end.flag);
  s(irq.error.enable);
  s(irq.error.flag);
  s(irq.unknown.enable);
  s(irq.unknown.flag);
  s(irq.start.enable);
  s(irq.start.flag);

  s(fifo.parameter);
  s(fifo.response);
  s(fifo.data);

  s(ssr.error);
  s(ssr.motorOn);
  s(ssr.seekError);
  s(ssr.idError);
  s(ssr.shellOpen);
  s(ssr.reading);
  s(ssr.seeking);
  s(ssr.playingCDDA);

  s(io.index);

  s(counter.sector);
  s(counter.cdda);
  s(counter.cdxa);
  s(counter.report);
}
