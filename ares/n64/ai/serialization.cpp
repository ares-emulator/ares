auto AI::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(fifo[0].address);
  s(fifo[1].address);

  s(io.dmaEnable);
  s(io.dmaAddress[0]);
  s(io.dmaAddress[1]);
  s(io.dmaAddressCarry);
  s(io.dmaLength[0]);
  s(io.dmaLength[1]);
  s(io.dmaCount);
  s(io.dmaOriginPc[0]);
  s(io.dmaOriginPc[1]);
  s(io.dacRate);
  s(io.bitRate);

  s(dac.frequency);
  s(dac.precision);
  s(dac.period);
  s(dac.left);
  s(dac.right);
  s(dac.decayFactor);

  if(s.reading() && stream->frequency() != dac.frequency) {
    stream->setFrequency(dac.frequency);
    updateDecay();
  }
}
