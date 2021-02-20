//computes the distance between the current LBA and seeking LBA
auto Disc::Drive::distance() const -> u32 {
  return 0;
}

auto Disc::Drive::clockSector() -> void {
  if(seeking && --seeking) return;

  if(self.ssr.reading) {
    if(likely(self.fd)) {  //there might not be a disc inserted here:
      self.debugger.read(lba.current);
      self.fd->seek(2448 * (abs(session->leadIn.lba) + lba.current++));
      self.fd->read({sector.data, 2448});
    }
    sector.offset = 0;

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
      if(mode.xaFilter) {
        if(sector.data[16] != cdxa->filter.file) return;
        if(sector.data[17] != cdxa->filter.channel) return;
      }
      if(mode.xaADPCM && (sector.data[18] & 0x44) == 0x44) {
        return cdxa->clockSector();
      }
      if(mode.xaFilter && (sector.data[18] & 0x44) == 0x44) {
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

    self.fifo.response.flush();
    self.fifo.response.write(self.status());

    self.irq.ready.flag = 1;
    self.irq.poll();
    return;
  }
}
