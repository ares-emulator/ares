auto APU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(square1);
  s(square2);
  s(wave);
  s(noise);
  s(sequencer);
  s(phase);
  s(cycle);
}
