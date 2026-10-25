// returns seek time in 75hz ticks, scaled by drive speed
auto Disc::Drive::distance() const -> u32 {
  auto a = lba.current;
  auto b = lba.request;
  auto d = a > b ? a - b : b - a;

  auto r = std::abs(
    std::sqrt((double)b + 150.0) -
    std::sqrt((double)a + 150.0)
  );

  auto t = 3.0 + r * 0.20 + std::sqrt(r) * 2.5 + (double)d / 60000.0;
  t *= 0.5;
  t = std::clamp(t, 1.0, 150.0);
  return (u32)t << mode.speed;
}

auto Disc::Drive::updateSubQ() -> void {
  u8 qbuf[12];
  self.fd->seek(2448ull * (CD::LeadInSectors + CD::LBAtoABA(lba.current)) + 2352 + 12);
  self.fd->read({qbuf, 12});

  const u8 adr = qbuf[0] & 0x0f;
  const u16 crc = (u16)qbuf[10] << 8 | qbuf[11];
  if(adr != 0x01 || CD::CRC16({qbuf, 10}) != crc) return;
  for(int i = 0; i < 10; i++) sector.subq[i] = qbuf[i];
}

auto Disc::Drive::clockSector() -> void {
  if(!likely(self.fd)) return;
  if(seeking || self.ssr.reading) self.ssr.motorOn = 1;
  if(seekDelay) {
    --seekDelay;
    return;
  }

  updateSubQ();

  if(seeking) {
    headerValid = 0;
    if(--seeking) {
      //aim to arrive a little early, this can be as much as 1-50 sectors on hardware, we pick a happy middle
      auto distance = lba.request - lba.current - 25;
      auto remaining = (s32)seeking;
      if(distance != 0) {
        auto step = (distance > 0 ? (distance + remaining - 1) / remaining : (distance - remaining + 1) / remaining);
        lba.current += distance > 0 ? min(step, distance) : max(step, distance);
      }

      return;
    }

    //logical seeking tends to be precise, with little or no error, so jump to our target here
    if(seekType == SeekType::SeekL) {
      lba.current = lba.request;
      updateSubQ();
    }

    if(seekType != SeekType::None) {
      const s32 target = lba.request;
      static constexpr s32 PregapSectors = 150;
      const bool beyondDisc = target < -PregapSectors
                           || target > session->leadOut.lba + CD::LeadOutSectors;
      bool inDataTrack = false;

      if(!beyondDisc) {
        if(auto trackID = session->inTrack(target)) {
          if(auto track = session->track(*trackID)) {
            inDataTrack = track->isData();
          }
        }
      }

      const bool ok = (seekType == SeekType::SeekP) ? !beyondDisc : (!beyondDisc && inDataTrack);

      if(ok) {
        if(seekType == SeekType::SeekL && inDataTrack) headerValid = 1;

        //correct for imprecise seek
        if(pendingOperation == PendingOperation::Play) {
          lba.current = lba.request;
          updateSubQ();
        }

        self.ssr.playingCDDA = 0;

        if(pendingOperation == PendingOperation::Read) {
          self.ssr.reading = 1;
          self.ssr.playingCDDA = 0;
        }

        if(pendingOperation == PendingOperation::Play) {
          self.ssr.reading = 1;
          self.ssr.playingCDDA = 1;
          cdda->playMode = CDDA::PlayMode::Normal;
          self.counter.report = system.frequency() / 75;
        }

        if(pendingOperation == PendingOperation::None) {
          self.queueResponse(ResponseType::Complete, {self.status()});
        }

        pendingOperation = PendingOperation::None;
        seekType = SeekType::None;
        return;
      }

      seekRetries++;
      u32 retryDelay = 1 << mode.speed;
      u32 retryLimit = beyondDisc ? 1 : 300;

      if(seekRetries > retryLimit) {
        self.ssr.seekError = 1;
        self.ssr.reading = 0;
        self.ssr.playingCDDA = 0;
        seeking = 0;
        seekDelay = 0;
        self.queueResponse(ResponseType::Error, {self.status(), beyondDisc ? ErrorCode_InvalidParameterValue : ErrorCode_SeekFailed});
        pendingOperation = PendingOperation::None;
        seekType = SeekType::None;
        standby = 0;
        return;
      }

      seeking = retryDelay;
      return;
    }

    return;
  }

  if(self.ssr.reading) {
    self.debugger.read(lba.current);
    self.fd->seek(2448ull * (CD::LeadInSectors + CD::LBAtoABA(lba.current++)));
    self.fd->read({sector.data, 2448});

    if(auto trackID = session->inTrack(lba.current)) {
      sector.track = *trackID;
    } else {
      sector.track = 0;
    }
    sector.offset = 0;
    headerValid = !self.ssr.playingCDDA;

    if(self.ssr.playingCDDA) {
      if(cdda->playMode == Disc::CDDA::PlayMode::FastForward) {
        s32 end = 0;
        if(auto trackID = session->inTrack(lba.current)) {
          if(auto track = session->track(*trackID)) {
            if(auto index = track->index(track->lastIndex)) {
              end = index->end;
            }
          }
        }

        lba.current = min(end, lba.current + 10);
      }
      if(cdda->playMode == Disc::CDDA::PlayMode::Rewind) {
        s32 start = 0;
        if(auto trackID = session->inTrack(lba.current)) {
          if(auto track = session->track(*trackID)) {
            if(auto index = track->index(1)) {
              start = index->lba;
            }
          }
        }

        lba.current = max(start, lba.current - 10);
      }
      return cdda->clockSector();
    }

    if(sector.data[15] == 0x02) {
      bool xaAudio = (sector.data[18] & 0x44) == 0x44;
      if(xaAudio && mode.xaFilter) {
        if(sector.data[16] != cdxa->filter.file) return;
        if(sector.data[17] != cdxa->filter.channel) return;
      }
      if(mode.xaADPCM && xaAudio) {
        return cdxa->clockSector();
      }
      if(mode.xaFilter && xaAudio) {
        return;
      }
    }

    //any remaining FIFO data is lost if a new sector is clocked
    //before all data from the previous sector was read by the CPU
    self.fifo.data.flush();

    if(mode.sectorSize == 0) {
      for(u32 offset : range(2048)) {
        self.fifo.data.write(sector.data[24 + offset]);
      }
    }

    if(mode.sectorSize == 1) {
      for(u32 offset : range(2340)) {
        self.fifo.data.write(sector.data[12 + offset]);
      }
    }

    self.queueResponse(ResponseType::Ready, {self.status()});
  }
}
