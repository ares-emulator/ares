auto SPU::readByte(u32 address) -> u32 {
  uint16 data = readHalf(address & ~1);
  return data >> 8 * (address & 1) & 0xff;
}

auto SPU::readHalf(u32 address) -> u32 {
  uint16 data = 0;

  u8 v = address >> 4 & 31;

  //volume left
  if((address & 0xffff'fe0f) == 0x1f80'1c00 && v < 24) {
    data.bit( 0,14) = voice[v].volume[0].level;
    data.bit(15)    = voice[v].volume[0].mode;
  }

  //volume right
  if((address & 0xffff'fe0f) == 0x1f80'1c02 && v < 24) {
    data.bit( 0,14) = voice[v].volume[1].level;
    data.bit(15)    = voice[v].volume[1].mode;
  }

  //ADPCM sample rate
  if((address & 0xffff'fe0f) == 0x1f80'1c04 && v < 24) {
    data.bit(0,15) = voice[v].adpcm.sampleRate;
  }

  //ADPCM start address
  if((address & 0xffff'fe0f) == 0x1f80'1c06 && v < 24) {
    data.bit(0,15) = voice[v].adpcm.startAddress >> 3;
  }

  //ADSR
  if((address & 0xffff'fe0f) == 0x1f80'1c08 && v < 24) {
    data.bit( 0, 3) = voice[v].sustain.level;
    data.bit( 4, 7) = voice[v].decay.rate;
    data.bit( 8,14) = voice[v].attack.rate;
    data.bit(15)    = voice[v].attack.exponential;
  }

  //ADSR
  if((address & 0xffff'fe0f) == 0x1f80'1c0a && v < 24) {
    data.bit( 0, 4) = voice[v].release.rate;
    data.bit( 5)    = voice[v].release.exponential;
    data.bit( 6,12) = voice[v].sustain.rate;
    data.bit(13)    = voice[v].sustain.unknown;
    data.bit(14)    = voice[v].sustain.decrease;
    data.bit(15)    = voice[v].sustain.exponential;
  }

  //current ADSR volume
  if((address & 0xffff'fe0f) == 0x1f80'1c0c && v < 24) {
    data.bit(0,15) = voice[v].adsr.volume;
  }

  //ADPCM repeat address
  if((address & 0xffff'fe0f) == 0x1f80'1c0e && v < 24) {
    data.bit(0,15) = voice[v].adpcm.repeatAddress >> 3;
  }

  //main volume left
  if(address == 0x1f80'1d80) {
    data.bit( 0,14) = volume[0].level;
    data.bit(15)    = volume[0].mode;
  }

  //main volume right
  if(address == 0x1f80'1d82) {
    data.bit( 0,14) = volume[1].level;
    data.bit(15)    = volume[1].mode;
  }

  //vLOUT
  if(address == 0x1f80'1d84) {
    data.bit(0,15) = reverb.volume.output[0];
  }

  //vROUT
  if(address == 0x1f80'1d86) {
    data.bit(0,15) = reverb.volume.output[1];
  }

  //KON
  if(address == 0x1f80'1d88) {
    for(uint v : range(24)) {
      data.bit(v) = voice[v].kon;
    }
  }

  //KOFF
  if(address == 0x1f80'1d8c) {
    for(uint v : range(24)) {
      data.bit(v) = voice[v].koff;
    }
  }

  //PMON
  if(address == 0x1f80'1d90) {
    for(uint v : range(24)) {
      data.bit(v) = voice[v].pmon;
    }
  }

  //NON
  if(address == 0x1f80'1d94) {
    for(uint v : range(24)) {
      data.bit(v) = voice[v].non;
    }
  }

  //EON
  if(address == 0x1f80'1d98) {
    for(uint v : range(24)) {
      data.bit(v) = voice[v].eon;
    }
  }

  //ENDX
  if(address == 0x1f80'1d9c) {
    for(uint v : range(24)) {
      data.bit(v) = voice[v].endx;
    }
  }

  //mBASE
  if(address == 0x1f80'1da2) {
    data.bit(0,15) = reverb.address.work >> 3;
  }

  //RAM IRQ address
  if(address == 0x1f80'1da4) {
    data.bit(0,15) = transfer.irqAddress >> 3;
  }

  //RAM transfer address
  if(address == 0x1f80'1da6) {
    data.bit(0,15) = transfer.address >> 3;
  }

  //RAM transfer data
  if(address == 0x1f80'1da8) {
    //write-only(?)
  }

  //SPUCNT
  if(address == 0x1f80'1daa) {
    data.bit( 0)    = cdda.enable;
    data.bit( 1)    = external.enable;
    data.bit( 2)    = cdda.reverb;
    data.bit( 3)    = external.reverb;
    data.bit( 4, 5) = transfer.mode;
    data.bit( 6)    = transfer.irqEnable;
    data.bit( 7)    = reverb.enable;
    data.bit( 8, 9) = noise.step;
    data.bit(10,13) = noise.shift;
    data.bit(14)    = master.mute;
    data.bit(15)    = master.enable;
  }

  //SPURAMCNT
  if(address == 0x1f80'1dac) {
    data.bit(0)    = transfer.unknown_0;
    data.bit(1,3)  = transfer.type;
    data.bit(4,15) = transfer.unknown_4_15;
  }

  //SPUSTAT
  if(address == 0x1f80'1dae) {
    data.bit(0)   = cdda.enable;
    data.bit(1)   = external.enable;
    data.bit(2)   = cdda.reverb;
    data.bit(3)   = external.reverb;
    data.bit(4,5) = transfer.mode;
    data.bit(7)   = transfer.mode.bit(1);  //unverified
  }

  //CD-DA volume left
  if(address == 0x1f80'1db0) {
    data.bit(0,15) = cdda.volume[0];
  }

  //CD-DA volume right
  if(address == 0x1f80'1db2) {
    data.bit(0,15) = cdda.volume[1];
  }

  //external volume left
  if(address == 0x1f80'1db4) {
    data.bit(0,15) = external.volume[0];
  }

  //external volume right
  if(address == 0x1f80'1db6) {
    data.bit(0,15) = external.volume[1];
  }

  //current volume left
  if(address == 0x1f80'1db8) {
    data.bit(0,15) = current.volume[0];
  }

  //current volume right
  if(address == 0x1f80'1dba) {
    data.bit(0,15) = current.volume[1];
  }

  //dAPF1
  if(address == 0x1f80'1dc0) {
    data.bit(0,15) = reverb.offset.apf1 >> 3;
  }

  //dAPF2
  if(address == 0x1f80'1dc2) {
    data.bit(0,15) = reverb.offset.apf2 >> 3;
  }

  //vIIR
  if(address == 0x1f80'1dc4) {
    data.bit(0,15) = reverb.volume.reflection[0];
  }

  //vCOMB1
  if(address == 0x1f80'1dc6) {
    data.bit(0,15) = reverb.volume.comb[0];
  }

  //vCOMB2
  if(address == 0x1f80'1dc8) {
    data.bit(0,15) = reverb.volume.comb[1];
  }

  //vCOMB3
  if(address == 0x1f80'1dca) {
    data.bit(0,15) = reverb.volume.comb[2];
  }

  //vCOMB4
  if(address == 0x1f80'1dcc) {
    data.bit(0,15) = reverb.volume.comb[3];
  }

  //vWALL
  if(address == 0x1f80'1dce) {
    data.bit(0,15) = reverb.volume.reflection[1];
  }

  //vAPF1
  if(address == 0x1f80'1dd0) {
    data.bit(0,15) = reverb.volume.apf[0];
  }

  //vAPF2
  if(address == 0x1f80'1dd2) {
    data.bit(0,15) = reverb.volume.apf[1];
  }

  //mLSAME
  if(address == 0x1f80'1dd4) {
    data.bit(0,15) = reverb.address.same1[0] >> 3;
  }

  //mRSAME
  if(address == 0x1f80'1dd6) {
    data.bit(0,15) = reverb.address.same1[1] >> 3;
  }

  //mLCOMB1
  if(address == 0x1f80'1dd8) {
    data.bit(0,15) = reverb.address.comb1[0] >> 3;
  }

  //mRCOMB1
  if(address == 0x1f80'1dda) {
    data.bit(0,15) = reverb.address.comb1[1] >> 3;
  }

  //mLCOMB2
  if(address == 0x1f80'1ddc) {
    data.bit(0,15) = reverb.address.comb2[0] >> 3;
  }

  //mRCOMB2
  if(address == 0x1f80'1dde) {
    data.bit(0,15) = reverb.address.comb2[1] >> 3;
  }

  //dLSAME
  if(address == 0x1f80'1de0) {
    data.bit(0,15) = reverb.address.same2[0] >> 3;
  }

  //dRSAME
  if(address == 0x1f80'1de2) {
    data.bit(0,15) = reverb.address.same2[1] >> 3;
  }

  //mLDIFF
  if(address == 0x1f80'1de4) {
    data.bit(0,15) = reverb.address.diff1[0] >> 3;
  }

  //mRDIFF
  if(address == 0x1f80'1de6) {
    data.bit(0,15) = reverb.address.diff1[1] >> 3;
  }

  //mLCOMB3
  if(address == 0x1f80'1de8) {
    data.bit(0,15) = reverb.address.comb3[0] >> 3;
  }

  //mRCOMB3
  if(address == 0x1f80'1dea) {
    data.bit(0,15) = reverb.address.comb3[1] >> 3;
  }

  //mLCOMB4
  if(address == 0x1f80'1dec) {
    data.bit(0,15) = reverb.address.comb4[0] >> 3;
  }

  //mRCOMB4
  if(address == 0x1f80'1dee) {
    data.bit(0,15) = reverb.address.comb4[1] >> 3;
  }

  //dLDIFF
  if(address == 0x1f80'1df0) {
    data.bit(0,15) = reverb.address.diff2[0] >> 3;
  }

  //dRDIFF
  if(address == 0x1f80'1df2) {
    data.bit(0,15) = reverb.address.diff2[1] >> 3;
  }

  //mLAPF1
  if(address == 0x1f80'1df4) {
    data.bit(0,15) = reverb.address.apf1[0] >> 3;
  }

  //mRAPF1
  if(address == 0x1f80'1df6) {
    data.bit(0,15) = reverb.address.apf1[1] >> 3;
  }

  //mLAPF2
  if(address == 0x1f80'1df8) {
    data.bit(0,15) = reverb.address.apf2[0] >> 3;
  }

  //mRAPF2
  if(address == 0x1f80'1dfa) {
    data.bit(0,15) = reverb.address.apf2[1] >> 3;
  }

  //vLIN
  if(address == 0x1f80'1dfc) {
    data.bit(0,15) = reverb.volume.input[0];
  }

  //vRIN
  if(address == 0x1f80'1dfe) {
    data.bit(0,15) = reverb.volume.input[1];
  }

  v = address >> 2 & 31;

  //current volume left
  if((address & 0xffff'ff83) == 0x1f80'1e00) {
    data.bit(0,15) = voice[v].current.volume[0];
  }

  //current volume right
  if((address & 0xffff'ff83) == 0x1f80'1e02) {
    data.bit(0,15) = voice[v].current.volume[1];
  }

  return data;
}

auto SPU::readWord(u32 address) -> u32 {
  uint16 data = readHalf(address & ~1 ^ 0) <<  0;
  return data | readHalf(address & ~1 ^ 2) << 16;
}

auto SPU::writeByte(u32 address, u32 value) -> void {
  if(address & 1) return;  //odd address writes are ignored
  writeHalf(address & ~1, value);
}

auto SPU::writeHalf(u32 address, u32 value) -> void {
  uint16 data = value;

  u8 v = address >> 4 & 31;

  //volume left
  if((address & 0xffff'fe0f) == 0x1f80'1c00 && v < 24) {
    voice[v].volume[0].level    = data.bit( 0,14);
    voice[v].volume[0].mode     = data.bit(15);

    voice[v].sweep[0].step      = data.bit( 0, 1);
    voice[v].sweep[0].shift     = data.bit( 2, 6);
    voice[v].sweep[0].unknown   = data.bit( 7,11);
    voice[v].sweep[0].phase     = data.bit(12);
    voice[v].sweep[0].direction = data.bit(13);
    voice[v].sweep[0].mode      = data.bit(14);
  }

  //volume right
  if((address & 0xffff'fe0f) == 0x1f80'1c02 && v < 24) {
    voice[v].volume[1].level    = data.bit( 0,14);
    voice[v].volume[1].mode     = data.bit(15);

    voice[v].sweep[1].step      = data.bit( 0, 1);
    voice[v].sweep[1].shift     = data.bit( 2, 6);
    voice[v].sweep[1].unknown   = data.bit( 7,11);
    voice[v].sweep[1].phase     = data.bit(12);
    voice[v].sweep[1].direction = data.bit(13);
    voice[v].sweep[1].mode      = data.bit(14);
  }

  //ADPCM sample rate
  if((address & 0xffff'fe0f) == 0x1f80'1c04 && v < 24) {
    voice[v].adpcm.sampleRate = data.bit(0,15);
  }

  //ADPCM start address
  if((address & 0xffff'fe0f) == 0x1f80'1c06 && v < 24) {
    voice[v].adpcm.startAddress = data.bit(0,15) << 3;
  }

  //ADSR
  if((address & 0xffff'fe0f) == 0x1f80'1c08 && v < 24) {
    voice[v].sustain.level      = data.bit( 0, 3);
    voice[v].decay.rate         = data.bit( 4, 7);
    voice[v].attack.rate        = data.bit( 8,14);
    voice[v].attack.exponential = data.bit(15);
  }

  //ADSR
  if((address & 0xffff'fe0f) == 0x1f80'1c0a && v < 24) {
    voice[v].release.rate        = data.bit( 0, 4);
    voice[v].release.exponential = data.bit( 5);
    voice[v].sustain.rate        = data.bit( 6,12);
    voice[v].sustain.unknown     = data.bit(13);
    voice[v].sustain.decrease    = data.bit(14);
    voice[v].sustain.exponential = data.bit(15);
  }

  //current ADSR volume
  if((address & 0xffff'fe0f) == 0x1f80'1c0c && v < 24) {
    voice[v].adsr.volume = data.bit(0,15);
  }

  //ADPCM repeat address
  if((address & 0xffff'fe0f) == 0x1f80'1c0e && v < 24) {
    voice[v].adpcm.repeatAddress = data.bit(0,15) << 3;
    voice[v].adpcm.ignoreLoopAddress = 1;
  }

  //main volume left
  if(address == 0x1f80'1d80) {
    volume[0].level    = data.bit( 0,14);
    volume[0].mode     = data.bit(15);

    sweep[0].step      = data.bit( 0, 1);
    sweep[0].shift     = data.bit( 2, 6);
    sweep[0].unknown   = data.bit( 7,11);
    sweep[0].phase     = data.bit(12);
    sweep[0].direction = data.bit(13);
    sweep[0].mode      = data.bit(14);
  }

  //main volume right
  if(address == 0x1f80'1d82) {
    volume[1].level    = data.bit( 0,14);
    volume[1].mode     = data.bit(15);

    sweep[1].step      = data.bit( 0, 1);
    sweep[1].shift     = data.bit( 2, 6);
    sweep[1].unknown   = data.bit( 7,11);
    sweep[1].phase     = data.bit(12);
    sweep[1].direction = data.bit(13);
    sweep[1].mode      = data.bit(14);
  }

  //vLOUT
  if(address == 0x1f80'1d84) {
    reverb.volume.output[0] = data.bit(0,15);
  }

  //vROUT
  if(address == 0x1f80'1d86) {
    reverb.volume.output[1] = data.bit(0,15);
  }

  //KON
  if(address == 0x1f80'1d88) {
    for(uint v : range(24)) {
      voice[v].kon = data.bit(v);
    }
  }

  //KOFF
  if(address == 0x1f80'1d8c) {
    for(uint v : range(24)) {
      voice[v].koff = data.bit(v);
    }
  }

  //PMON
  if(address == 0x1f80'1d90) {
    for(uint v : range(24)) {
      voice[v].pmon = data.bit(v);
    }
  }

  //NON
  if(address == 0x1f80'1d94) {
    for(uint v : range(24)) {
      voice[v].non = data.bit(v);
    }
  }

  //EON
  if(address == 0x1f80'1d98) {
    for(uint v : range(24)) {
      voice[v].eon = data.bit(v);
    }
  }

  //ENDX
  if(address == 0x1f80'1d9c) {
    for(uint v : range(24)) {
      voice[v].endx = data.bit(v);
    }
  }

  //mBASE
  if(address == 0x1f80'1da2) {
    reverb.address.work = data.bit(0,15) << 3;
  }

  //RAM IRQ address
  if(address == 0x1f80'1da4) {
    transfer.irqAddress = data.bit(0,15) << 3;
  }

  //RAM transfer address
  if(address == 0x1f80'1da6) {
    transfer.address = data.bit(0,15) << 3;
  }

  //RAM transfer data
  if(address == 0x1f80'1da8) {
    fifoWrite(data);
  }

  //SPUCNT
  if(address == 0x1f80'1daa) {
    cdda.enable        = data.bit( 0);
    external.enable    = data.bit( 1);
    cdda.reverb        = data.bit( 2);
    external.reverb    = data.bit( 3);
    transfer.mode      = data.bit( 4, 5);
    transfer.irqEnable = data.bit( 6);
    reverb.enable      = data.bit( 7);
    noise.step         = data.bit( 8, 9);
    noise.shift        = data.bit(10,13);
    master.mute        = data.bit(14);
    master.enable      = data.bit(15);

    if(transfer.mode == 0) fifoReset();
    if(transfer.mode == 1) fifoManualWrite();
  }

  //SPURAMCNT
  if(address == 0x1f80'1dac) {
    transfer.unknown_0    = data.bit(0);
    transfer.type         = data.bit(1, 3);
    transfer.unknown_4_15 = data.bit(4,15);
  }

  //SPUSTAT
  if(address == 0x1f80'1dae) {
    //read-only
  }

  //CD-DA volume left
  if(address == 0x1f80'1db0) {
    cdda.volume[0] = data.bit(0,15);
  }

  //CD-DA volume right
  if(address == 0x1f80'1db2) {
    cdda.volume[1] = data.bit(0,15);
  }

  //external volume left
  if(address == 0x1f80'1db4) {
    external.volume[0] = data.bit(0,15);
  }

  //external volume right
  if(address == 0x1f80'1db6) {
    external.volume[1] = data.bit(0,15);
  }

  //current volume left
  if(address == 0x1f80'1db8) {
    current.volume[0] = data.bit(0,15);
  }

  //current volume right
  if(address == 0x1f80'1dba) {
    current.volume[1] = data.bit(0,15);
  }

  //dAPF1
  if(address == 0x1f80'1dc0) {
    reverb.offset.apf1 = data.bit(0,15) << 3;
  }

  //dAPF2
  if(address == 0x1f80'1dc2) {
    reverb.offset.apf2 = data.bit(0,15) << 3;
  }

  //vIIR
  if(address == 0x1f80'1dc4) {
    reverb.volume.reflection[0] = data.bit(0,15);
  }

  //vCOMB1
  if(address == 0x1f80'1dc6) {
    reverb.volume.comb[0] = data.bit(0,15);
  }

  //vCOMB2
  if(address == 0x1f80'1dc8) {
    reverb.volume.comb[1] = data.bit(0,15);
  }

  //vCOMB3
  if(address == 0x1f80'1dca) {
    reverb.volume.comb[2] = data.bit(0,15);
  }

  //vCOMB4
  if(address == 0x1f80'1dcc) {
    reverb.volume.comb[3] = data.bit(0,15);
  }

  //vWALL
  if(address == 0x1f80'1dce) {
    reverb.volume.reflection[1] = data.bit(0,15);
  }

  //vAPF1
  if(address == 0x1f80'1dd0) {
    reverb.volume.apf[0] = data.bit(0,15);
  }

  //vAPF2
  if(address == 0x1f80'1dd2) {
    reverb.volume.apf[1] = data.bit(0,15);
  }

  //mLSAME
  if(address == 0x1f80'1dd4) {
    reverb.address.same1[0] = data.bit(0,15) << 3;
  }

  //mRSAME
  if(address == 0x1f80'1dd6) {
    reverb.address.same1[1] = data.bit(0,15) << 3;
  }

  //mLCOMB1
  if(address == 0x1f80'1dd8) {
    reverb.address.comb1[0] = data.bit(0,15) << 3;
  }

  //mRCOMB1
  if(address == 0x1f80'1dda) {
    reverb.address.comb1[1] = data.bit(0,15) << 3;
  }

  //mLCOMB2
  if(address == 0x1f80'1ddc) {
    reverb.address.comb2[0] = data.bit(0,15) << 3;
  }

  //mRCOMB2
  if(address == 0x1f80'1dde) {
    reverb.address.comb2[1] = data.bit(0,15) << 3;
  }

  //dLSAME
  if(address == 0x1f80'1de0) {
    reverb.address.same2[0] = data.bit(0,15) << 3;
  }

  //dRSAME
  if(address == 0x1f80'1de2) {
    reverb.address.same2[1] = data.bit(0,15) << 3;
  }

  //mLDIFF
  if(address == 0x1f80'1de4) {
    reverb.address.diff1[0] = data.bit(0,15) << 3;
  }

  //mRDIFF
  if(address == 0x1f80'1de6) {
    reverb.address.diff1[1] = data.bit(0,15) << 3;
  }

  //mLCOMB3
  if(address == 0x1f80'1de8) {
    reverb.address.comb3[0] = data.bit(0,15) << 3;
  }

  //mRCOMB3
  if(address == 0x1f80'1dea) {
    reverb.address.comb3[1] = data.bit(0,15) << 3;
  }

  //mLCOMB4
  if(address == 0x1f80'1dec) {
    reverb.address.comb4[0] = data.bit(0,15) << 3;
  }

  //mRCOMB4
  if(address == 0x1f80'1dee) {
    reverb.address.comb4[1] = data.bit(0,15) << 3;
  }

  //dLDIFF
  if(address == 0x1f80'1df0) {
    reverb.address.diff2[0] = data.bit(0,15) << 3;
  }

  //dRDIFF
  if(address == 0x1f80'1df2) {
    reverb.address.diff2[1] = data.bit(0,15) << 3;
  }

  //mLAPF1
  if(address == 0x1f80'1df4) {
    reverb.address.apf1[0] = data.bit(0,15) << 3;
  }

  //mRAPF1
  if(address == 0x1f80'1df6) {
    reverb.address.apf1[1] = data.bit(0,15) << 3;
  }

  //mLAPF2
  if(address == 0x1f80'1df8) {
    reverb.address.apf2[0] = data.bit(0,15) << 3;
  }

  //mRAPF2
  if(address == 0x1f80'1dfa) {
    reverb.address.apf2[1] = data.bit(0,15) << 3;
  }

  //vLIN
  if(address == 0x1f80'1dfc) {
    reverb.volume.input[0] = data.bit(0,15);
  }

  //vRIN
  if(address == 0x1f80'1dfe) {
    reverb.volume.input[1] = data.bit(0,15);
  }

  v = address >> 2 & 31;

  //current volume left
  if((address & 0xffff'ff83) == 0x1f80'1e00 && v < 24) {
    voice[v].current.volume[0] = data.bit(0,15);
  }

  //current volume right
  if((address & 0xffff'ff83) == 0x1f80'1e02 && v < 24) {
    voice[v].current.volume[1] = data.bit(0,15);
  }
}

auto SPU::writeWord(u32 address, u32 value) -> void {
  writeHalf(address & ~1 ^ 0, value >>  0);
  writeHalf(address & ~1 ^ 2, value >> 16);
}
