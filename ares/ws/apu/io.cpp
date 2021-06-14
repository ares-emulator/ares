auto APU::portRead(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x004a ... 0x004c:  //SDMA_SRC
    data = dma.s.source.byte(address - 0x004a);
    break;

  case 0x004e ... 0x0050:  //SDMA_LEN
    data = dma.s.length.byte(address - 0x004e);
    break;

  case 0x0052:  //SDMA_CTRL
    data.bit(0,1) = dma.r.rate;
    data.bit(2)   = dma.r.unknown;
    data.bit(3)   = dma.r.loop;
    data.bit(4)   = dma.r.target;
    data.bit(6)   = dma.r.direction;
    data.bit(7)   = dma.r.enable;
    break;

  case 0x006a:  //SND_HYPER_CTRL
    data.bit(0,1) = channel5.r.volume;
    data.bit(2,3) = channel5.r.scale;
    data.bit(4,6) = channel5.r.speed;
    data.bit(7)   = channel5.r.enable;
    break;

  case 0x006b:  //SND_HYPER_CHAN_CTRL
    data.bit(0,3) = channel5.r.unknown;
    data.bit(5)   = channel5.r.leftEnable;
    data.bit(6)   = channel5.r.rightEnable;
    break;

  case 0x0080 ... 0x0081:  //SND_CH1_PITCH
    data = channel1.r.pitch.byte(address - 0x0080);
    break;

  case 0x0082 ... 0x0083:  //SND_CH2_PITCH
    data = channel2.r.pitch.byte(address - 0x0082);
    break;

  case 0x0084 ... 0x0085:  //SND_CH3_PITCH
    data = channel3.r.pitch.byte(address - 0x0084);
    break;

  case 0x0086 ... 0x0087:  //SND_CH4_PITCH
    data = channel4.r.pitch.byte(address - 0x0086);
    break;

  case 0x0088:  //SND_CH1_VOL
    data.bit(0,3) = channel1.r.volumeRight;
    data.bit(4,7) = channel1.r.volumeLeft;
    break;

  case 0x0089:  //SND_CH2_VOL
    data.bit(0,3) = channel2.r.volumeRight;
    data.bit(4,7) = channel2.r.volumeLeft;
    break;

  case 0x008a:  //SND_CH3_VOL
    data.bit(0,3) = channel3.r.volumeRight;
    data.bit(4,7) = channel3.r.volumeLeft;
    break;

  case 0x008b:  //SND_CH4_VOL
    data.bit(0,3) = channel4.r.volumeRight;
    data.bit(4,7) = channel4.r.volumeLeft;
    break;

  case 0x008c:  //SND_SWEEP_VALUE
    data = channel3.r.sweepValue;
    break;

  case 0x008d:  //SND_SWEEP_TIME
    data = channel3.r.sweepTime;
    break;

  case 0x008e:  //SND_NOISE
    data.bit(0,2) = channel4.r.noiseMode;
    data.bit(3)   = 0;  //noiseReset always reads as zero
    data.bit(4)   = channel4.r.noiseUpdate;
    break;

  case 0x008f:  //SND_WAVE_BASE
    data = r.waveBase;
    break;

  case 0x0090:  //SND_CTRL
    data.bit(0) = channel1.r.enable;
    data.bit(1) = channel2.r.enable;
    data.bit(2) = channel3.r.enable;
    data.bit(3) = channel4.r.enable;
    data.bit(5) = channel2.r.voice;
    data.bit(6) = channel3.r.sweep;
    data.bit(7) = channel4.r.noise;
    break;

  case 0x0091:  //SND_OUTPUT
    data.bit(0)   = r.speakerEnable;
    data.bit(1,2) = r.speakerShift;
    data.bit(3)   = r.headphonesEnable;
    data.bit(7)   = r.headphonesConnected;
    break;

  case 0x0092 ... 0x0093:  //SND_RANDOM
    data = channel4.s.noiseLFSR.byte(address - 0x0092);
    break;

  case 0x0094:  //SND_VOICE_CTRL
    data.bit(0,1) = channel2.r.voiceEnableRight;
    data.bit(2,3) = channel2.r.voiceEnableLeft;
    break;

  case 0x0095:  //SND_HYPERVOICE
    data = channel5.s.data;
    break;

  case 0x009e:  //SND_VOLUME
    if(!SoC::ASWAN()) {
      data.bit(0,1) = r.masterVolume;
    }
    break;

  }

  return data;
}

auto APU::portWrite(n16 address, n8 data) -> void {
  switch(address) {

  case 0x004a ... 0x004c:  //SDMA_SRC
    dma.r.source.byte(address - 0x004a) = data;
    break;

  case 0x004e ... 0x0050:  //SDMA_LEN
    dma.r.length.byte(address - 0x004e) = data;
    break;

  case 0x0052: {  //SDMA_CTRL
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
  } break;

  case 0x006a:  //SND_HYPER_CTRL
    channel5.r.volume = data.bit(0,1);
    channel5.r.scale  = data.bit(2,3);
    channel5.r.speed  = data.bit(4,6);
    channel5.r.enable = data.bit(7);
    break;

  case 0x006b:  //SND_HYPER_CHAN_CTRL
    channel5.r.unknown     = data.bit(0,3);
    channel5.r.leftEnable  = data.bit(5);
    channel5.r.rightEnable = data.bit(6);
    break;

  case 0x0080 ... 0x0081:  //SND_CH1_PITCH
    channel1.r.pitch.byte(address - 0x0080) = data;
    break;

  case 0x0082 ... 0x0083:  //SND_CH2_PITCH
    channel2.r.pitch.byte(address - 0x0082) = data;
    break;

  case 0x0084 ... 0x0085:  //SND_CH3_PITCH
    channel3.r.pitch.byte(address - 0x0084) = data;
    break;

  case 0x0086 ... 0x0087:  //SND_CH4_PITCH
    channel4.r.pitch.byte(address - 0x0086) = data;
    break;

  case 0x0088:  //SND_CH1_VOL
    channel1.r.volumeRight = data.bit(0,3);
    channel1.r.volumeLeft  = data.bit(4,7);
    break;

  case 0x0089:  //SND_CH2_VOL
    channel2.r.volumeRight = data.bit(0,3);
    channel2.r.volumeLeft  = data.bit(4,7);
    break;

  case 0x008a:  //SND_CH3_VOL
    channel3.r.volumeRight = data.bit(0,3);
    channel3.r.volumeLeft  = data.bit(4,7);
    break;

  case 0x008b:  //SND_CH4_VOL
    channel4.r.volumeRight = data.bit(0,3);
    channel4.r.volumeLeft  = data.bit(4,7);
    break;

  case 0x008c:  //SND_SWEEP_VALUE
    channel3.r.sweepValue = data;
    break;

  case 0x008d:  //SND_SWEEP_TIME
    channel3.r.sweepTime = data.bit(0,4);
    break;

  case 0x008e:  //SND_NOISE
    channel4.r.noiseMode   = data.bit(0,2);
    channel4.r.noiseReset  = data.bit(3);
    channel4.r.noiseUpdate = data.bit(4);
    break;

  case 0x008f:  //SND_WAVE_BASE
    r.waveBase = data;
    break;

  case 0x0090:  //SND_CTRL
    channel1.r.enable = data.bit(0);
    channel2.r.enable = data.bit(1);
    channel3.r.enable = data.bit(2);
    channel4.r.enable = data.bit(3);
    channel2.r.voice  = data.bit(5);
    channel3.r.sweep  = data.bit(6);
    channel4.r.noise  = data.bit(7);
    break;

  case 0x0091:  //SND_OUTPUT
    r.speakerEnable    = data.bit(0);
    r.speakerShift     = data.bit(1,2);
    r.headphonesEnable = data.bit(3);
    break;

  case 0x0094:  //SND_VOICE_CTRL
    channel2.r.voiceEnableRight = data.bit(0,1);
    channel2.r.voiceEnableLeft  = data.bit(2,3);
    break;

  case 0x009e:  //SND_VOLUME
    if(!SoC::ASWAN()) {
      r.masterVolume = data.bit(0,1);
      ppu.updateIcons();
    }
    break;

  }

  return;
}
