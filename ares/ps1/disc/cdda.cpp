auto Disc::CDDA::load(Node::Object parent) -> void {
//stream = parent->append<Node::Audio::Stream>("CD-DA");
//stream->setChannels(2);
//stream->setFrequency(44100);
}

auto Disc::CDDA::unload(Node::Object parent) -> void {
//parent->remove(stream);
//stream.reset();
}

auto Disc::CDDA::clockSector() -> void {
}

auto Disc::CDDA::clockSample() -> void {
  s16 left  = 0;
  s16 right = 0;

  auto track = drive->session->track(drive->sector.track);
  auto isData = track ? track->isData() : true;

  if(self.ssr.reading && self.ssr.playingCDDA && !isData) {
    left  |= drive->sector.data[drive->sector.offset++] << 0;
    left  |= drive->sector.data[drive->sector.offset++] << 8;
    right |= drive->sector.data[drive->sector.offset++] << 0;
    right |= drive->sector.data[drive->sector.offset++] << 8;
  }

  if(self.ssr.reading && self.ssr.playingCDDA && !isData) {
    s32 absLeft = left < 0 ? -(s32)left : left;
    s32 absRight = right < 0 ? -(s32)right : right;
    peak = max(peak, max(absLeft, absRight));
  }

  if(self.audio.mute) {
    sample.left  = 0;
    sample.right = 0;
  } else {
    s32 mixLeft  = (left * self.audio.volume[0] >> 7) + (right * self.audio.volume[2] >> 7);
    s32 mixRight = (left * self.audio.volume[1] >> 7) + (right * self.audio.volume[3] >> 7);
    sample.left  = (s16)std::clamp<s32>((mixLeft  * 30294) >> 15, -31900, 31900);
    sample.right = (s16)std::clamp<s32>((mixRight * 30294) >> 15, -31900, 31900);
  }

//stream->sample(sample.left / 32768.0, sample.right / 32768.0);
}
