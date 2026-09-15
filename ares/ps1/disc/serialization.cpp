auto Disc::serialize(serializer& s) -> void {
  s(controllerVersion);
  s(lidOpen);
  s(manualLidControl);
  s(drive.lba.current);
  s(drive.lba.hold);
  s(drive.lba.request);
  s(drive.lba.seek);
  s(drive.lba.start);
  s(drive.lba.pending);
  s(drive.physical.position);
  s(drive.physical.age);
  s(drive.physical.carry);
  s(drive.physical.pending);
  s(drive.sector.data);
  s(drive.sector.subq);
  s(drive.sector.track);
  s(drive.mode.cdda);
  s(drive.mode.autoPause);
  s(drive.mode.report);
  s(drive.mode.xaFilter);
  s(drive.mode.ignore);
  s(drive.mode.sectorSize);
  s(drive.mode.xaADPCM);
  s(drive.mode.speed);
  s(drive.header);
  s(drive.headerValid);
  s(drive.spinUp);
  s(drive.shellOpening);
  s(drive.resetDelay);
  s(drive.seeking);
  s(drive.seekInterval);
  s(drive.seekVisible);
  s(drive.streamStarting);
  s(drive.activeStatusCleared);
  s(drive.startingStatus);
  s(drive.speedChange);
  s(drive.seekRandom);
  s(drive.seekType);
  s(drive.pendingOperation);
  s(drive.sessionChange.number);
  s(drive.sessionChange.remaining);
  s(drive.sessionChange.pending);

  s(audio.frames);
  s(audio.sample.left);
  s(audio.sample.right);
  s(audio.mute);
  s(audio.muteADPCM);
  s(audio.volume);
  s(audio.volumeLatch);

  s((u32&)cdda.playMode);
  s(cdda.reportDelay);
  s(cdda.reportFrame);
  s(cdda.scanStep);
  s(cdda.started);
  s(cdda.playTrack);
  s(cdda.holdLBA);
  s(cdda.autoPausePending);

  s(cdxa.filter.file);
  s(cdxa.filter.channel);
  s(cdxa.current.file);
  s(cdxa.current.channel);
  s(cdxa.current.selected);
  s(cdxa.previousSamples);
  s(cdxa.resampleRing);
  s(cdxa.resamplePosition);
  s(cdxa.resampleStep);

  s(command.first.command);
  s(command.first.pending);
  s(command.first.counter);
  s(command.second.command);
  s(command.second.pending);
  s(command.second.counter);
  s(command.elapsed);
  s(command.active);

  s(irq.flag);
  s(irq.mask);
  s(irq.acknowledgeAge);

  s(fifo.parameter);
  s(fifo.response);
  s(fifo.responseLatch);
  s(fifo.responsePosition);
  auto buffer = [&](FIFO::DataBuffer& data) {
    s(data.bytes);
    s(data.length);
    s(data.position);
    s(data.padding);
    s(data.exhausted);
  };
  buffer(fifo.data);
  for(auto& sector : fifo.sectors) buffer(sector);
  s(fifo.sectorRead);
  s(fifo.sectorWrite);
  s(fifo.sectorPending);
  s(fifo.hostLoaded);
  s(fifo.sectorFile);
  s(fifo.sectorChannel);
  s(fifo.sectorNeedsFilter);
  s(fifo.deferred.type);
  s(fifo.deferred.data);
  s(fifo.deferred.counter);
  s(fifo.deferred.scheduled);

  s(ssr.error);
  s(ssr.motorOn);
  s(ssr.seekError);
  s(ssr.idError);
  s(ssr.shellOpen);
  s(ssr.reading);
  s(ssr.playingCDDA);

  s(io.index);
  s(io.soundMapEnable);
  s(io.sectorBufferReadRequest);
  s(io.sectorBufferWriteRequest);

  s(counter.sector);
  s(counter.audio);
  s(clockAccumulator);
  if(s.reading()) synchronizeLidSetting();
}
