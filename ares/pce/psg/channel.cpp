auto PSG::Channel::power(u32 id) -> void {
  this->id = id;
  io = {};
}

template<int index, int step> auto PSG::Channel::run() -> n5 {
  for(u32 n : range(step)) {
    if(!io.direct && --io.wavePeriod == 0) {
      io.wavePeriod = io.waveFrequency;
      io.waveOffset++;
      io.waveSample = io.waveBuffer[io.waveOffset];
    }

    //Only channels 4-5 support noise
    if(index >= 4 && io.noiseEnable) {
      if(--io.noisePeriod == 0) {
        io.noisePeriod = ~io.noiseFrequency << 7;
        io.noiseSample = nall::random() & 1 ? ~0 : 0;
      }
    }
  }

  return index >= 4 && io.noiseEnable ? io.noiseSample : io.waveSample;
}
