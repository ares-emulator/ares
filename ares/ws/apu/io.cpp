auto APU::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x004a ... 0x004c:  //SDMA_SRC
    data = dma.state.source.byte(address - 0x004a);
    break;

  case 0x004e ... 0x0050:  //SDMA_LEN
    data = dma.state.length.byte(address - 0x004e);
    break;

  case 0x0052:  //SDMA_CTRL
    data.bit(0,1) = dma.io.rate;
    data.bit(2)   = dma.io.unknown;
    data.bit(3)   = dma.io.loop;
    data.bit(4)   = dma.io.target;
    data.bit(6)   = dma.io.direction;
    data.bit(7)   = dma.io.enable;
    break;

  case 0x006a:  //SND_HYPER_CTRL
    data.bit(0,1) = channel5.io.volume;
    data.bit(2,3) = channel5.io.scale;
    data.bit(4,6) = channel5.io.speed;
    data.bit(7)   = channel5.io.enable;
    break;

  case 0x006b:  //SND_HYPER_CHAN_CTRL
    data.bit(0,3) = channel5.io.unknown;
    data.bit(5)   = channel5.io.leftEnable;
    data.bit(6)   = channel5.io.rightEnable;
    break;

  case 0x0080 ... 0x0081:  //SND_CH1_PITCH
    data = channel1.io.pitch.byte(address - 0x0080);
    break;

  case 0x0082 ... 0x0083:  //SND_CH2_PITCH
    data = channel2.io.pitch.byte(address - 0x0082);
    break;

  case 0x0084 ... 0x0085:  //SND_CH3_PITCH
    data = channel3.io.pitch.byte(address - 0x0084);
    break;

  case 0x0086 ... 0x0087:  //SND_CH4_PITCH
    data = channel4.io.pitch.byte(address - 0x0086);
    break;

  case 0x0088:  //SND_CH1_VOL
    data.bit(0,3) = channel1.io.volumeRight;
    data.bit(4,7) = channel1.io.volumeLeft;
    break;

  case 0x0089:  //SND_CH2_VOL
    data.bit(0,3) = channel2.io.volumeRight;
    data.bit(4,7) = channel2.io.volumeLeft;
    break;

  case 0x008a:  //SND_CH3_VOL
    data.bit(0,3) = channel3.io.volumeRight;
    data.bit(4,7) = channel3.io.volumeLeft;
    break;

  case 0x008b:  //SND_CH4_VOL
    data.bit(0,3) = channel4.io.volumeRight;
    data.bit(4,7) = channel4.io.volumeLeft;
    break;

  case 0x008c:  //SND_SWEEP_VALUE
    data = channel3.io.sweepValue;
    break;

  case 0x008d:  //SND_SWEEP_TIME
    data = channel3.io.sweepTime;
    break;

  case 0x008e:  //SND_NOISE
    data.bit(0,2) = channel4.io.noiseMode;
    data.bit(3)   = 0;  //noiseReset always reads as zero
    data.bit(4)   = channel4.io.noiseUpdate;
    break;

  case 0x008f:  //SND_WAVE_BASE
    data = io.waveBase;
    break;

  case 0x0090:  //SND_CTRL
    data.bit(0) = channel1.io.enable;
    data.bit(1) = channel2.io.enable;
    data.bit(2) = channel3.io.enable;
    data.bit(3) = channel4.io.enable;
    data.bit(5) = channel2.io.voice;
    data.bit(6) = channel3.io.sweep;
    data.bit(7) = channel4.io.noise;
    break;

  case 0x0091:  //SND_OUTPUT
    data.bit(0)   = io.speakerEnable;
    data.bit(1,2) = io.speakerShift;
    data.bit(3)   = io.headphonesEnable;
    data.bit(7)   = io.headphonesConnected;
    break;

  case 0x0092 ... 0x0093:  //SND_RANDOM
    data = channel4.state.noiseLFSR.byte(address - 0x0092);
    break;

  case 0x0094:  //SND_VOICE_CTRL
    data.bit(0,1) = channel2.io.voiceEnableRight;
    data.bit(2,3) = channel2.io.voiceEnableLeft;
    break;

  case 0x0095:  //SND_HYPERVOICE
    data = channel5.state.data;
    break;

  case 0x009e:  //SND_VOLUME
    if(!SoC::ASWAN()) {
      data.bit(0,1) = io.masterVolume;
    }
    break;

  }

  return data;
}

auto APU::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x004a ... 0x004c:  //SDMA_SRC
    dma.io.source.byte(address - 0x004a) = data;
    break;

  case 0x004e ... 0x0050:  //SDMA_LEN
    dma.io.length.byte(address - 0x004e) = data;
    break;

  case 0x0052: {  //SDMA_CTRL
    bool trigger = !dma.io.enable && data.bit(7);
    dma.io.rate      = data.bit(0,1);
    dma.io.unknown   = data.bit(2);
    dma.io.loop      = data.bit(3);
    dma.io.target    = data.bit(4);
    dma.io.direction = data.bit(6);
    dma.io.enable    = data.bit(7);
    if(trigger) {
      dma.state.source = dma.io.source;
      dma.state.length = dma.io.length;
    }
  } break;

  case 0x006a:  //SND_HYPER_CTRL
    channel5.io.volume = data.bit(0,1);
    channel5.io.scale  = data.bit(2,3);
    channel5.io.speed  = data.bit(4,6);
    channel5.io.enable = data.bit(7);
    break;

  case 0x006b:  //SND_HYPER_CHAN_CTRL
    channel5.io.unknown     = data.bit(0,3);
    channel5.io.leftEnable  = data.bit(5);
    channel5.io.rightEnable = data.bit(6);
    break;

  case 0x0080 ... 0x0081:  //SND_CH1_PITCH
    channel1.io.pitch.byte(address - 0x0080) = data;
    break;

  case 0x0082 ... 0x0083:  //SND_CH2_PITCH
    channel2.io.pitch.byte(address - 0x0082) = data;
    break;

  case 0x0084 ... 0x0085:  //SND_CH3_PITCH
    channel3.io.pitch.byte(address - 0x0084) = data;
    break;

  case 0x0086 ... 0x0087:  //SND_CH4_PITCH
    channel4.io.pitch.byte(address - 0x0086) = data;
    break;

  case 0x0088:  //SND_CH1_VOL
    channel1.io.volumeRight = data.bit(0,3);
    channel1.io.volumeLeft  = data.bit(4,7);
    break;

  case 0x0089:  //SND_CH2_VOL
    channel2.io.volumeRight = data.bit(0,3);
    channel2.io.volumeLeft  = data.bit(4,7);
    break;

  case 0x008a:  //SND_CH3_VOL
    channel3.io.volumeRight = data.bit(0,3);
    channel3.io.volumeLeft  = data.bit(4,7);
    break;

  case 0x008b:  //SND_CH4_VOL
    channel4.io.volumeRight = data.bit(0,3);
    channel4.io.volumeLeft  = data.bit(4,7);
    break;

  case 0x008c:  //SND_SWEEP_VALUE
    channel3.io.sweepValue = data;
    break;

  case 0x008d:  //SND_SWEEP_TIME
    channel3.io.sweepTime = data.bit(0,4);
    break;

  case 0x008e:  //SND_NOISE
    channel4.io.noiseMode   = data.bit(0,2);
    channel4.io.noiseReset  = data.bit(3);
    channel4.io.noiseUpdate = data.bit(4);
    break;

  case 0x008f:  //SND_WAVE_BASE
    io.waveBase = data;
    break;

  case 0x0090:  //SND_CTRL
    channel1.io.enable = data.bit(0);
    channel2.io.enable = data.bit(1);
    channel3.io.enable = data.bit(2);
    channel4.io.enable = data.bit(3);
    channel2.io.voice  = data.bit(5);
    channel3.io.sweep  = data.bit(6);
    channel4.io.noise  = data.bit(7);
    break;

  case 0x0091:  //SND_OUTPUT
    io.speakerEnable    = data.bit(0);
    io.speakerShift     = data.bit(1,2);
    io.headphonesEnable = data.bit(3);
    break;

  case 0x0094:  //SND_VOICE_CTRL
    channel2.io.voiceEnableRight = data.bit(0,1);
    channel2.io.voiceEnableLeft  = data.bit(2,3);
    break;

  case 0x009e:  //SND_VOLUME
    if(!SoC::ASWAN()) {
      io.masterVolume = data.bit(0,1);
      ppu.updateIcons();
    }
    break;

  }

  return;
}
