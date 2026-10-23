auto Disc::Audio::pushFrame(s16 left, s16 right, bool xa) -> void {
  frames.write(u64(u16(left)) | u64(u16(right)) << 16 | u64(xa) << 32);
}

auto Disc::Audio::clockSample() -> void {
  u64 frame = frames.read(0);
  if(mute || (muteADPCM && (frame >> 32 & 1))) {
    sample = {};
    return;
  }
  s16 left = frame, right = frame >> 16;
  sample.left = mix(left, right, 0);
  sample.right = mix(left, right, 1);
}
