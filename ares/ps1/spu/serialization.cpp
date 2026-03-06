auto SPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(ram);

  s(master.enable);
  s(master.unmute);

  s(noise.step);
  s(noise.shift);
  s(noise.level);
  s(noise.count);

  s(transfer.mode);
  s(transfer.type);
  s(transfer.address);
  s(transfer.current);
  s(transfer.unknown_0);
  s(transfer.unknown_4_15);

  s(irq.enable);
  s(irq.flag);
  s(irq.address);

  s(envelope.counter);
  s(envelope.rate);
  s(envelope.decreasing);
  s(envelope.exponential);

  for(auto& v : volume) {
    s(v.counter);
    s(v.rate);
    s(v.decreasing);
    s(v.exponential);

    s(v.active);
    s(v.sweep);
    s(v.negative);
    s(v.level);
    s(v.current);
  }

  s(cdaudio.enable);
  s(cdaudio.reverb);
  s(cdaudio.volume[0]);
  s(cdaudio.volume[1]);

  s(external.enable);
  s(external.reverb);
  s(external.volume[0]);
  s(external.volume[1]);

  s(current.volume[0]);
  s(current.volume[1]);

  s(reverb.enable);
  s(reverb.vLOUT);
  s(reverb.vROUT);
  s(reverb.mBASE);
  s(reverb.FB_SRC_A);
  s(reverb.FB_SRC_B);
  s(reverb.IIR_ALPHA);
  s(reverb.ACC_COEF_A);
  s(reverb.ACC_COEF_B);
  s(reverb.ACC_COEF_C);
  s(reverb.ACC_COEF_D);
  s(reverb.IIR_COEF);
  s(reverb.FB_ALPHA);
  s(reverb.FB_X);
  s(reverb.IIR_DEST_A0);
  s(reverb.IIR_DEST_A1);
  s(reverb.ACC_SRC_A0);
  s(reverb.ACC_SRC_A1);
  s(reverb.ACC_SRC_B0);
  s(reverb.ACC_SRC_B1);
  s(reverb.IIR_SRC_A0);
  s(reverb.IIR_SRC_A1);
  s(reverb.IIR_DEST_B0);
  s(reverb.IIR_DEST_B1);
  s(reverb.ACC_SRC_C0);
  s(reverb.ACC_SRC_C1);
  s(reverb.ACC_SRC_D0);
  s(reverb.ACC_SRC_D1);
  s(reverb.IIR_SRC_B1);  //misordered
  s(reverb.IIR_SRC_B0);  //misordered
  s(reverb.MIX_DEST_A0);
  s(reverb.MIX_DEST_A1);
  s(reverb.MIX_DEST_B0);
  s(reverb.MIX_DEST_B1);
  s(reverb.IN_COEF_L);
  s(reverb.IN_COEF_R);
  s(reverb.lastInput[0]);
  s(reverb.lastInput[1]);
  s(reverb.lastOutput[0]);
  s(reverb.lastOutput[1]);
  for(auto& d : reverb.downsampleBuffer) {
    for(auto& e : d) s(e);
  }
  for(auto& u : reverb.upsampleBuffer) {
    for(auto& v : u) s(v);
  }
  s(reverb.upsampleBuffer[0]);
  s(reverb.upsampleBuffer[1]);
  s(reverb.resamplePosition);
  s(reverb.baseAddress);
  s(reverb.currentAddress);

  for(auto& v : voice) {
    s(v.adpcm.startAddress);
    s(v.adpcm.repeatAddress);
    s(v.adpcm.sampleRate);
    s(v.adpcm.currentAddress);
    s(v.adpcm.hasSamples);
    s(v.adpcm.ignoreLoopAddress);
    s(v.adpcm.lastSamples[0]);
    s(v.adpcm.lastSamples[1]);
    s(v.adpcm.previousSamples[0]);
    s(v.adpcm.previousSamples[1]);
    s(v.adpcm.previousSamples[2]);
    for(auto& c : v.adpcm.currentSamples) s(c);
  
    s(v.block.shift);
    s(v.block.filter);
    s(v.block.loopEnd);
    s(v.block.loopRepeat);
    s(v.block.loopStart);
    for(auto& b : v.block.brr) s(b);

    s(v.attack.rate);
    s(v.attack.exponential);

    s(v.decay.rate);

    s(v.sustain.level);
    s(v.sustain.exponential);
    s(v.sustain.decrease);
    s(v.sustain.rate);
    s(v.sustain.unknown);

    s(v.release.exponential);
    s(v.release.rate);

    s((u32&)v.adsr.phase);
    s(v.adsr.volume);
    s(v.adsr.lastVolume);
    s(v.adsr.target);

    s(v.current.volume[0]);
    s(v.current.volume[1]);

    for(auto& v : v.volume) {
      s(v.counter);
      s(v.rate);
      s(v.decreasing);
      s(v.exponential);

      s(v.active);
      s(v.sweep);
      s(v.negative);
      s(v.level);
      s(v.current);
    }

    s(v.envelope.counter);
    s(v.envelope.rate);
    s(v.envelope.decreasing);
    s(v.envelope.exponential);

    s(v.counter);
    s(v.pmon);
    s(v.non);
    s(v.eon);
    s(v.kon);
    s(v.koff);
    s(v.endx);
  }

  s(capture.address);

  s(fifo);
}
