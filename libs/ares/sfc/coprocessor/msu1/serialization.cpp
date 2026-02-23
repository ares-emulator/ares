auto MSU1::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(io.dataSeekOffset);
  s(io.dataReadOffset);

  s(io.audioPlayOffset);
  s(io.audioLoopOffset);

  s(io.audioTrack);
  s(io.audioVolume);

  s(io.audioResumeTrack);
  s(io.audioResumeOffset);

  s(io.audioError);
  s(io.audioPlay);
  s(io.audioRepeat);
  s(io.audioBusy);
  s(io.dataBusy);

  dataOpen();
  audioOpen();
}
