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

  if(self.ssr.reading && self.ssr.playingCDDA) {
    left  |= drive->sector.data[drive->sector.offset++] << 0;
    left  |= drive->sector.data[drive->sector.offset++] << 8;
    right |= drive->sector.data[drive->sector.offset++] << 0;
    right |= drive->sector.data[drive->sector.offset++] << 8;
  }

  if(self.audio.mute) {
    sample.left  = 0;
    sample.right = 0;
  } else {
    //each channel is saturated; but the combination of each channel is not and may overflow
    sample.left  = sclamp<16>(left * self.audio.volume[0] >> 7) + sclamp<16>(right * self.audio.volume[2] >> 7);
    sample.right = sclamp<16>(left * self.audio.volume[1] >> 7) + sclamp<16>(right * self.audio.volume[3] >> 7);
  }

//stream->sample(sample.left / 32768.0, sample.right / 32768.0);
}
