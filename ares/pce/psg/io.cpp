auto PSG::write(uint4 address, uint8 data) -> void {
  if(address == 0x00) {
    io.channel = data.bit(0,2);
  }

  if(address == 0x01) {
    io.volumeRight = data.bit(0,3);
    io.volumeLeft  = data.bit(4,7);
  }

  if(address >= 0x02 && address <= 0x06 && io.channel <= 5) {
    channel[io.channel].write(address, data);
  }

  if(address == 0x07 && io.channel >= 4 && io.channel <= 5) {
    channel[io.channel].write(address, data);
  }

  if(address == 0x08) {
    io.lfoFrequency = data;
  }

  if(address == 0x09) {
    io.lfoControl = data.bit(0,1);
    io.lfoEnable  = data.bit(7);
    if(io.lfoEnable) {
      channel[1].io.waveSample = channel[1].io.waveBuffer[channel[1].io.waveOffset = 0];
    }
  }
}

auto PSG::Channel::write(uint4 address, uint8 data) -> void {
  if(address == 0x02) {
    io.waveFrequency.bit(0,7) = data.bit(0,7);
    io.wavePeriod = io.waveFrequency;
  }

  if(address == 0x03) {
    io.waveFrequency.bit(8,11) = data.bit(0,3);
    io.wavePeriod = io.waveFrequency;
  }

  if(address == 0x04) {
    if(io.direct && !data.bit(6)) {
      io.waveOffset = 0;
      io.waveSample = io.waveBuffer[io.waveOffset];
    }
    if(!io.enable && data.bit(7) && !data.bit(6)) {
      io.waveOffset++;
      io.waveSample = io.waveBuffer[io.waveOffset];
    }
    io.volume = data.bit(0,4);
    io.direct = data.bit(6);
    io.enable = data.bit(7);
  }

  if(address == 0x05) {
    io.volumeRight = data.bit(0,3);
    io.volumeLeft  = data.bit(4,7);
  }

  if(address == 0x06) {
    if(!io.direct) {
      io.waveBuffer[io.waveOffset] = data.bit(0,4);
      if(!io.enable) io.waveOffset++;
    }
    if(io.enable) {
      io.waveSample = data.bit(0,4);
    }
  }

  //channels 4 and 5 only
  if(address == 0x07) {
    if(!io.noiseEnable && data.bit(7)) {
      io.noisePeriod = ~data.bit(0,4) << 7;
      io.noiseSample = 0;
    }
    io.noiseFrequency = data.bit(0,4);
    io.noiseEnable    = data.bit(7);
  }
}
