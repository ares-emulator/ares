auto DSP::envelopeRun(Voice& v) -> void {
  int envelope = v.envelope;

  if(v.envelopeMode == Envelope::Release) {  //60%
    envelope -= 0x8;
    if(envelope < 0) envelope = 0;
    v.envelope = envelope;
    return;
  }

  int rate;
  int envelopeData = v.adsr1;
  if(latch.adsr0.bit(7)) {  //99% ADSR
    if(v.envelopeMode >= Envelope::Decay) {  //99%
      envelope--;
      envelope -= envelope >> 8;
      rate = envelopeData & 0x1f;
      if(v.envelopeMode == Envelope::Decay) {  //1%
        rate = latch.adsr0.bit(4,6) * 2 + 16;
      }
    } else {  //env_attack
      rate = latch.adsr0.bit(0,3) * 2 + 1;
      envelope += rate < 31 ? 0x20 : 0x400;
    }
  } else {  //GAIN
    envelopeData = v.gain;
    int mode = envelopeData >> 5;
    if(mode < 4) {  //direct
      envelope = envelopeData << 4;
      rate = 31;
    } else {
      rate = envelopeData & 0x1f;
      if(mode == 4) {  //4: linear decrease
        envelope -= 0x20;
      } else if(mode < 6) {  //5: exponential decrease
        envelope--;
        envelope -= envelope >> 8;
      } else {  //6, 7: linear increase
        envelope += 0x20;
        if(mode > 6 && (uint)v._envelope >= 0x600) {
          envelope += 0x8 - 0x20;  //7: two-slope linear increase
        }
      }
    }
  }

  //sustain level
  if((envelope >> 8) == (envelopeData >> 5) && v.envelopeMode == Envelope::Decay) {
    v.envelopeMode = Envelope::Sustain;
  }
  v._envelope = envelope;

  //uint cast because linear decrease underflowing also triggers this
  if((uint)envelope > 0x7ff) {
    envelope = (envelope < 0 ? 0 : 0x7ff);
    if(v.envelopeMode == Envelope::Attack) v.envelopeMode = Envelope::Decay;
  }

  if(counterPoll(rate)) v.envelope = envelope;
}
