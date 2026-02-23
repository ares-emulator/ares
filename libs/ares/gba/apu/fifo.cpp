auto APU::FIFO::sample() -> void {
  output = active;
}

auto APU::FIFO::read() -> void {
  if(samplesRead < 4) active = buffer.byte(samplesRead++);
  if(size() > 0) {
    if(samplesRead >= 4) {
      samplesRead = 0;
      buffer = samples[rdoffset++];
    }
  }
}

auto APU::FIFO::write(n2 address, n8 byte) -> void {
  samples[wroffset].byte(address) = byte;
  if(address == 3) wroffset++;
}

auto APU::FIFO::reset() -> void {
  buffer = 0;
  for(auto& word : samples) word = 0;
  active = 0;
  output = 0;

  rdoffset = 0;
  wroffset = 0;
  samplesRead = 0;
}

auto APU::FIFO::power() -> void {
  reset();

  lenable = 0;
  renable = 0;
  timer = 0;
}
