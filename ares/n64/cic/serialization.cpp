
auto CIC::serialize(serializer& s) -> void {
  s(fifo);
  s(seed);
  s(checksum);
  s(type);
  s(region);
  s(state);
  s(challengeAlgo);
}
