auto APU::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(pulse1);
  s(pulse2);
  s(triangle);
  s(dmc);
  s(frame);
  s(enabledChannels);
}

auto APU::Envelope::serialize(serializer& s) -> void {
  s(speed);
  s(useSpeedAsVolume);
  s(loopMode);
  s(reloadDecay);
  s(decayCounter);
  s(decayVolume);
}

auto APU::Sweep::serialize(serializer& s) -> void {
  s(shift);
  s(decrement);
  s(period);
  s(counter);
  s(enable);
  s(reload);
  s(pulsePeriod);
}

auto APU::Pulse::serialize(serializer& s) -> void {
  s(envelope);
  s(sweep);
  s(lengthCounter);
  s(periodCounter);
  s(duty);
  s(dutyCounter);
  s(period);
}

auto APU::Triangle::serialize(serializer& s) -> void {
  s(lengthCounter);
  s(periodCounter);
  s(linearLength);
  s(haltLengthCounter);
  s(period);
  s(stepCounter);
  s(linearLengthCounter);
  s(reloadLinear);
}

auto APU::Noise::serialize(serializer& s) -> void {
  s(envelope);
  s(lengthCounter);
  s(periodCounter);
  s(period);
  s(shortMode);
  s(lfsr);
}

auto APU::DMC::serialize(serializer& s) -> void {
  s(lengthCounter);
  s(periodCounter);
  s(dmaDelayCounter);
  s(irqPending);
  s(period);
  s(irqEnable);
  s(loopMode);
  s(dacLatch);
  s(addressLatch);
  s(lengthLatch);
  s(readAddress);
  s(bitCounter);
  s(dmaBufferValid);
  s(dmaBuffer);
  s(sampleValid);
  s(sample);
}

auto APU::FrameCounter::serialize(serializer& s) -> void {
  s(irqPending);
  s(mode);
  s(counter);
  s(divider);
}
