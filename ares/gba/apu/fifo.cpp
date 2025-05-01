auto APU::FIFO::sample() -> void {
  output = active;
}

auto APU::FIFO::read() -> void {
  if(size == 0) return;
  size--;
  active = samples[rdoffset];
  rdoffset = (rdoffset + 1) % 28;
}

auto APU::FIFO::write(i8 byte) -> void {
  if(size == 28) {
    rdoffset = (rdoffset + 1) % 28;
  } else {
    size++;
  }
  samples[wroffset] = byte;
  wroffset = (wroffset + 1) % 28;
}

auto APU::FIFO::reset() -> void {
  for(auto& byte : samples) byte = 0;
  active = 0;
  output = 0;

  rdoffset = 0;
  wroffset = 0;
  size = 0;
}

auto APU::FIFO::power() -> void {
  reset();

  lenable = 0;
  renable = 0;
  timer = 0;
}
