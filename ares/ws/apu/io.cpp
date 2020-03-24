auto APU::portRead(uint16 address) -> uint8 {
  uint8 data;

  //SDMA_SRC
  if(address == 0x004a) return dma.s.source.bit( 0, 7);
  if(address == 0x004b) return dma.s.source.bit( 8,15);
  if(address == 0x004c) return dma.s.source.bit(16,19);

  //SDMA_LEN
  if(address == 0x004e) return dma.s.length.bit( 0, 7);
  if(address == 0x004f) return dma.s.length.bit( 8,15);
  if(address == 0x0050) return dma.s.length.bit(16,19);

  //SDMA_CTRL
  if(address == 0x0052) {
    data.bit(0,1) = dma.r.rate;
    data.bit(2)   = dma.r.unknown;
    data.bit(3)   = dma.r.loop;
    data.bit(4)   = dma.r.target;
    data.bit(6)   = dma.r.direction;
    data.bit(7)   = dma.r.enable;
    return data;
  }

  //SND_HYPER_CTRL
  if(address == 0x006a) {
    data.bit(0,1) = channel5.r.volume;
    data.bit(2,3) = channel5.r.scale;
    data.bit(4,6) = channel5.r.speed;
    data.bit(7)   = channel5.r.enable;
    return data;
  }

  //SND_HYPER_CHAN_CTRL
  if(address == 0x006b) {
    data.bit(0,3) = channel5.r.unknown;
    data.bit(5)   = channel5.r.leftEnable;
    data.bit(6)   = channel5.r.rightEnable;
    return data;
  }

  //SND_CH1_PITCH
  if(address == 0x0080) return channel1.r.pitch.bit(0, 7);
  if(address == 0x0081) return channel1.r.pitch.bit(8,11);

  //SND_CH2_PITCH
  if(address == 0x0082) return channel2.r.pitch.bit(0, 7);
  if(address == 0x0083) return channel2.r.pitch.bit(8,11);

  //SND_CH3_PITCH
  if(address == 0x0084) return channel3.r.pitch.bit(0, 7);
  if(address == 0x0085) return channel3.r.pitch.bit(8,11);

  //SND_CH4_PITCH
  if(address == 0x0086) return channel4.r.pitch.bit(0, 7);
  if(address == 0x0087) return channel4.r.pitch.bit(8,11);

  //SND_CH1_VOL
  if(address == 0x0088) {
    data.bit(0,3) = channel1.r.volumeRight;
    data.bit(4,7) = channel1.r.volumeLeft;
    return data;
  }

  //SND_CH2_VOL
  if(address == 0x0089) {
    data.bit(0,3) = channel2.r.volumeRight;
    data.bit(4,7) = channel2.r.volumeLeft;
    return data;
  }

  //SND_CH3_VOL
  if(address == 0x008a) {
    data.bit(0,3) = channel3.r.volumeRight;
    data.bit(4,7) = channel3.r.volumeLeft;
    return data;
  }

  //SND_CH4_VOL
  if(address == 0x008b) {
    data.bit(0,3) = channel4.r.volumeRight;
    data.bit(4,7) = channel4.r.volumeLeft;
    return data;
  }

  //SND_SWEEP_VALUE
  if(address == 0x008c) {
    return channel3.r.sweepValue;
  }

  //SND_SWEEP_TIME
  if(address == 0x008d) {
    return channel3.r.sweepTime;
  }

  //SND_NOISE
  if(address == 0x008e) {
    data.bit(0,2) = channel4.r.noiseMode;
    data.bit(3)   = 0;  //noiseReset always reads as zero
    data.bit(4)   = channel4.r.noiseUpdate;
    return data;
  }

  //SND_WAVE_BASE
  if(address == 0x008f) {
    return r.waveBase;
  }

  //SND_CTRL
  if(address == 0x0090) {
    data.bit(0) = channel1.r.enable;
    data.bit(1) = channel2.r.enable;
    data.bit(2) = channel3.r.enable;
    data.bit(3) = channel4.r.enable;
    data.bit(5) = channel2.r.voice;
    data.bit(6) = channel3.r.sweep;
    data.bit(7) = channel4.r.noise;
    return data;
  }

  //SND_OUTPUT
  if(address == 0x0091) {
    data.bit(0)   = r.speakerEnable;
    data.bit(1,2) = r.speakerShift;
    data.bit(3)   = r.headphonesEnable;
    data.bit(7)   = r.headphonesConnected;
    return data;
  }

  //SND_RANDOM
  if(address == 0x0092) return channel4.s.noiseLFSR.bit(0, 7);
  if(address == 0x0093) return channel4.s.noiseLFSR.bit(8,14);

  //SND_VOICE_CTRL
  if(address == 0x0094) {
    data.bit(0,1) = channel2.r.voiceEnableRight;
    data.bit(2,3) = channel2.r.voiceEnableLeft;
    return data;
  }

  //SND_HYPERVOICE
  if(address == 0x0095) {
    return channel5.s.data;
  }

  //SND_VOLUME
  if(address == 0x009e) {
    if(!SoC::ASWAN()) {
      data.bit(0,1) = r.masterVolume;
    }
    return data;
  }

  return data;
}

auto APU::portWrite(uint16 address, uint8 data) -> void {
  //SDMA_SRC
  if(address == 0x004a) { dma.r.source.bit( 0, 7) = data.bit(0,7); return; }
  if(address == 0x004b) { dma.r.source.bit( 8,15) = data.bit(0,7); return; }
  if(address == 0x004c) { dma.r.source.bit(16,19) = data.bit(0,3); return; }

  //SDMA_LEN
  if(address == 0x004e) { dma.r.length.bit( 0, 7) = data.bit(0,7); return; }
  if(address == 0x004f) { dma.r.length.bit( 8,15) = data.bit(0,7); return; }
  if(address == 0x0050) { dma.r.length.bit(16,19) = data.bit(0,3); return; }

  //SDMA_CTRL
  if(address == 0x0052) {
    bool trigger = !dma.r.enable && data.bit(7);
    dma.r.rate      = data.bit(0,1);
    dma.r.unknown   = data.bit(2);
    dma.r.loop      = data.bit(3);
    dma.r.target    = data.bit(4);
    dma.r.direction = data.bit(6);
    dma.r.enable    = data.bit(7);
    if(trigger) {
      dma.s.source = dma.r.source;
      dma.s.length = dma.r.length;
    }
    return;
  }

  //SND_HYPER_CTRL
  if(address == 0x006a) {
    channel5.r.volume = data.bit(0,1);
    channel5.r.scale  = data.bit(2,3);
    channel5.r.speed  = data.bit(4,6);
    channel5.r.enable = data.bit(7);
    return;
  }

  //SND_HYPER_CHAN_CTRL
  if(address == 0x006b) {
    channel5.r.unknown     = data.bit(0,3);
    channel5.r.leftEnable  = data.bit(5);
    channel5.r.rightEnable = data.bit(6);
    return;
  }

  //SND_CH1_PITCH
  if(address == 0x0080) { channel1.r.pitch.bit(0, 7) = data.bit(0,7); return; }
  if(address == 0x0081) { channel1.r.pitch.bit(8,11) = data.bit(0,3); return; }

  //SND_CH2_PITCH
  if(address == 0x0082) { channel2.r.pitch.bit(0, 7) = data.bit(0,7); return; }
  if(address == 0x0083) { channel2.r.pitch.bit(8,11) = data.bit(0,3); return; }

  //SND_CH3_PITCH
  if(address == 0x0084) { channel3.r.pitch.bit(0, 7) = data.bit(0,7); return; }
  if(address == 0x0085) { channel3.r.pitch.bit(8,11) = data.bit(0,3); return; }

  //SND_CH4_PITCH
  if(address == 0x0086) { channel4.r.pitch.bit(0, 7) = data.bit(0,7); return; }
  if(address == 0x0087) { channel4.r.pitch.bit(8,11) = data.bit(0,3); return; }

  //SND_CH1_VOL
  if(address == 0x0088) {
    channel1.r.volumeRight = data.bit(0,3);
    channel1.r.volumeLeft  = data.bit(4,7);
    return;
  }

  //SND_CH2_VOL
  if(address == 0x0089) {
    channel2.r.volumeRight = data.bit(0,3);
    channel2.r.volumeLeft  = data.bit(4,7);
    return;
  }

  //SND_CH3_VOL
  if(address == 0x008a) {
    channel3.r.volumeRight = data.bit(0,3);
    channel3.r.volumeLeft  = data.bit(4,7);
    return;
  }

  //SND_CH4_VOL
  if(address == 0x008b) {
    channel4.r.volumeRight = data.bit(0,3);
    channel4.r.volumeLeft  = data.bit(4,7);
    return;
  }

  //SND_SWEEP_VALUE
  if(address == 0x008c) {
    channel3.r.sweepValue = data;
    return;
  }

  //SND_SWEEP_TIME
  if(address == 0x008d) {
    channel3.r.sweepTime = data.bit(0,4);
    return;
  }

  //SND_NOISE
  if(address == 0x008e) {
    channel4.r.noiseMode   = data.bit(0,2);
    channel4.r.noiseReset  = data.bit(3);
    channel4.r.noiseUpdate = data.bit(4);
    return;
  }

  //SND_WAVE_BASE
  if(address == 0x008f) {
    r.waveBase = data;
    return;
  }

  //SND_CTRL
  if(address == 0x0090) {
    channel1.r.enable = data.bit(0);
    channel2.r.enable = data.bit(1);
    channel3.r.enable = data.bit(2);
    channel4.r.enable = data.bit(3);
    channel2.r.voice  = data.bit(5);
    channel3.r.sweep  = data.bit(6);
    channel4.r.noise  = data.bit(7);
    return;
  }

  //SND_OUTPUT
  if(address == 0x0091) {
    r.speakerEnable    = data.bit(0);
    r.speakerShift     = data.bit(1,2);
    r.headphonesEnable = data.bit(3);
    return;
  }

  //SND_VOICE_CTRL
  if(address == 0x0094) {
    channel2.r.voiceEnableRight = data.bit(0,1);
    channel2.r.voiceEnableLeft  = data.bit(2,3);
    return;
  }

  //SND_VOLUME
  if(address == 0x009e) {
    if(!SoC::ASWAN()) {
      r.masterVolume = data.bit(0,1);
      ppu.updateIcons();
    }
    return;
  }

  return;
}
