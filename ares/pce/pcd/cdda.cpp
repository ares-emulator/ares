auto PCD::CDDA::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("CD-DA");
  stream->setChannels(2);
  stream->setFrequency(44100);
}

auto PCD::CDDA::unload() -> void {
  stream = {};
}

auto PCD::CDDA::clockSector() -> void {
  if(!drive->inCDDA()) return;
  if(drive->paused()) return;
  if(drive->stopped()) return;

  drive->read();
  sample.offset = 0;
}

auto PCD::CDDA::clockSample() -> void {
  sample.left  = 0;
  sample.right = 0;

  if(drive->playing()) {
    sample.left  |= drive->sector[sample.offset++] << 0;
    sample.left  |= drive->sector[sample.offset++] << 8;
    sample.right |= drive->sector[sample.offset++] << 0;
    sample.right |= drive->sector[sample.offset++] << 8;
  }

  stream->sample(sample.left / 32768.0, sample.right / 32768.0);
}

auto PCD::CDDA::power() -> void {
  sample = {};
}
