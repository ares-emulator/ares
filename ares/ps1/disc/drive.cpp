//computes the distance between the current LBA and seeking LBA
auto Disc::Drive::distance() const -> uint {
  return 0;
}

auto Disc::Drive::clockSector() -> void {
  if(self.ssr.reading) {
    self.fd->seek(2448 * (abs(session->leadIn.lba) + lba.current));
    self.fd->read(sector.data, 2448);
    sector.offset = 0;

    if(!self.ssr.playingCDDA) {
      sector.offset += 24;  //skip sync header and mode 2 form header
      lba.current++;

      self.fifo.response.flush();
      self.fifo.response.write(self.status());

      self.irq.ready.flag = 1;
      self.irq.poll();
    } else if(cdda->playMode == Disc::CDDA::PlayMode::Normal) {
      lba.current++;
    } else if(cdda->playMode == Disc::CDDA::PlayMode::FastForward) {
      int end = 0;
      if(auto trackID = session->inTrack(lba.current)) {
        if(auto track = session->track(*trackID)) {
          if(auto index = track->index(track->lastIndex)) {
            end = index->end;
          }
        }
      }
      lba.current = min(end, lba.current + 10);
    } else if(cdda->playMode == Disc::CDDA::PlayMode::Rewind) {
      int start = 0;
      if(auto trackID = session->inTrack(lba.current)) {
        if(auto track = session->track(*trackID)) {
          if(auto index = track->index(1)) {
            start = index->lba;
          }
        }
      }
      lba.current = max(start, lba.current - 10);
    }
  }
}
