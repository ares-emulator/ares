auto Disc::CDDA::load(Node::Object parent) -> void {
//stream = parent->append<Node::Audio::Stream>("CD-DA");
//stream->setChannels(2);
//stream->setFrequency(44100);
}

auto Disc::CDDA::unload(Node::Object parent) -> void {
//parent->remove(stream);
//stream.reset();
}

auto Disc::CDDA::beginReport() -> void {
  reportDelay = 60;  // R1/R2 reference startup delay, counted in eligible audio sectors.
  reportFrame = 0xff;
}

auto Disc::CDDA::peak(u8 channel) const -> u16 {
  u32 maximum = 0;
  u32 referencePeak = 0;
  for(u32 frame : range(588)) {
    u32 offset = frame * 4 + channel * 2;
    s32 value = s16(drive->sector.data[offset] | u16(drive->sector.data[offset + 1]) << 8);
    maximum = max(maximum, u32(value < 0 ? -value : value));
    if(value > 0) referencePeak = max(referencePeak, u32(value));
  }
  // H2 absolute peak; the unrepresentable abs(-32768) case uses the user's reference fallback policy.
  return (u16(channel) << 15) | (maximum > 0x7fff ? referencePeak : maximum);
}

auto Disc::CDDA::report(bool validSubQ) -> void {
  if(!self.ssr.playingCDDA || !drive->mode.report || !validSubQ) return;
  const auto* q = drive->sector.data + 2352 + 12;
  if(q[0] & 0x40) return;
  if(reportDelay) {
    reportDelay--;
    return;
  }
  u8 frame = q[9] >> 4;
  if(frame == reportFrame) return;
  reportFrame = frame;
  bool relative = q[9] & 0x10;
  u16 level = peak(q[8] & 1);
  self.queueResponse(ResponseType::Ready, {
    self.status(), q[1], q[2], q[relative ? 3 : 7], u8(q[relative ? 4 : 8] | (relative ? 0x80 : 0)),
    q[relative ? 5 : 9], u8(level), u8(level >> 8)
  });
}

auto Disc::CDDA::clockSector(bool validSubQ) -> void {
  report(validSubQ);
  if(self.audio.mute || autoPausePending) return;
  u32 count = drive->mode.speed ? 294 : 588;
  u32 stride = drive->mode.speed ? 8 : 4;
  while(self.audio.frames.size() + count > self.audio.frames.capacity()) self.audio.frames.read(0);
  for(u32 frame : range(count)) {
    u32 offset = frame * stride;
    s16 left = drive->sector.data[offset] | u16(drive->sector.data[offset + 1]) << 8;
    s16 right = drive->sector.data[offset + 2] | u16(drive->sector.data[offset + 3]) << 8;
    self.audio.pushFrame(left, right);
  }
}
