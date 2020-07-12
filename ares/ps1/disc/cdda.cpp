auto Disc::CDDA::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("CD-DA");
  stream->setChannels(2);
  stream->setFrequency(44100);
}

auto Disc::CDDA::unload() -> void {
  stream.reset();
}

auto Disc::CDDA::clockSector() -> void {
}

auto Disc::CDDA::clockSample() -> void {
  sample.left  = 0;
  sample.right = 0;

  if(self.ssr.reading && self.ssr.playingCDDA) {
    sample.left  |= drive->sector.data[drive->sector.offset++] << 0;
    sample.left  |= drive->sector.data[drive->sector.offset++] << 8;
    sample.right |= drive->sector.data[drive->sector.offset++] << 0;
    sample.right |= drive->sector.data[drive->sector.offset++] << 8;
  }

  stream->sample(sample.left / 32768.0, sample.right / 32768.0);
}
