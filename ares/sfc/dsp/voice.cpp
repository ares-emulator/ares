inline auto DSP::voiceOutput(Voice& v, uint1 channel) -> void {
  //apply left/right volume
  int amp = latch.output * v.volume[channel] >> 7;

  //add to output total
  master.output[channel] += amp;
  master.output[channel] = sclamp<16>(master.output[channel]);

  //optionally add to echo total
  if(v._echo) {
    echo.output[channel] += amp;
    echo.output[channel] = sclamp<16>(echo.output[channel]);
  }
}

auto DSP::voice1(Voice& v) -> void {
  brr._address = (brr._bank << 8) + (brr._source << 2);
  brr._source = v.source;
}

auto DSP::voice2(Voice& v) -> void {
  //read sample pointer (ignored if not needed)
  uint16 address = brr._address;
  if(!v.keyonDelay) address += 2;
  brr._nextAddress.byte(0) = apuram[address++];
  brr._nextAddress.byte(1) = apuram[address++];
  latch.adsr0 = v.adsr0;

  //read pitch, spread over two clocks
  latch.pitch = v.pitch & 0xff;
}

auto DSP::voice3(Voice& v) -> void {
  voice3a(v);
  voice3b(v);
  voice3c(v);
}

auto DSP::voice3a(Voice& v) -> void {
  latch.pitch |= v.pitch & ~0xff;
}

auto DSP::voice3b(Voice& v) -> void {
  brr._byte   = apuram[uint16(v.brrAddress + v.brrOffset)];
  brr._header = apuram[uint16(v.brrAddress)];
}

auto DSP::voice3c(Voice& v) -> void {
  //pitch modulation using previous voice's output

  if(v._modulate) {
    latch.pitch += (latch.output >> 5) * latch.pitch >> 10;
  }

  if(v.keyonDelay) {
    //get ready to start BRR decoding on next sample
    if(v.keyonDelay == 5) {
      v.brrAddress = brr._nextAddress;
      v.brrOffset = 1;
      v.bufferOffset = 0;
      brr._header = 0;  //header is ignored on this sample
    }

    //envelope is never run during KON
    v.envelope = 0;
    v._envelope = 0;

    //disable BRR decoding until last three samples
    v.gaussianOffset = 0;
    v.keyonDelay--;
    if(v.keyonDelay & 3) v.gaussianOffset = 0x4000;

    //pitch is never added during KON
    latch.pitch = 0;
  }

  //gaussian interpolation
  int output = gaussianInterpolate(v);

  //noise
  if(v._noise) {
    output = (int16)(noise.lfsr << 1);
  }

  //apply envelope
  latch.output = output * v.envelope >> 11 & ~1;
  v.envx = v.envelope >> 4;

  //immediate silence due to end of sample or soft reset
  if(master.reset || brr._header.bit(0,1) == 1) {
    v.envelopeMode = Envelope::Release;
    v.envelope = 0;
  }

  if(clock.sample) {
    //KOFF
    if(v._keyoff) {
      v.envelopeMode = Envelope::Release;
    }

    //KON
    if(v._keyon) {
      v.keyonDelay = 5;
      v.envelopeMode = Envelope::Attack;
    }
  }

  //run envelope for next sample
  if(!v.keyonDelay) envelopeRun(v);
}

auto DSP::voice4(Voice& v) -> void {
  //decode BRR
  v._looped = 0;
  if(v.gaussianOffset >= 0x4000) {
    brrDecode(v);
    v.brrOffset += 2;
    if(v.brrOffset >= 9) {
      //start decoding next BRR block
      v.brrAddress = uint16(v.brrAddress + 9);
      if(brr._header.bit(0)) {
        v.brrAddress = brr._nextAddress;
        v._looped = 1;
      }
      v.brrOffset = 1;
    }
  }

  //apply pitch
  v.gaussianOffset = (v.gaussianOffset & 0x3fff) + latch.pitch;

  //keep from getting too far ahead (when using pitch modulation)
  if(v.gaussianOffset > 0x7fff) v.gaussianOffset = 0x7fff;

  //output left
  voiceOutput(v, 0);
}

auto DSP::voice5(Voice& v) -> void {
  //output right
  voiceOutput(v, 1);

  //ENDX, OUTX, ENVX won't update if you wrote to them 1-2 clocks earlier
  v._end |= v._looped;

  //clear bit in ENDX if KON just began
  if(v.keyonDelay == 5) v._end = 0;
}

auto DSP::voice6(Voice& v) -> void {
  latch.outx = latch.output >> 8;
}

auto DSP::voice7(Voice& v) -> void {
  for(uint n : range(8)) registers[0x7c].bit(n) = voice[n]._end;
  latch.envx = v.envx;
}

auto DSP::voice8(Voice& v) -> void {
  registers[v.index | 0x09] = latch.outx;
}

auto DSP::voice9(Voice& v) -> void {
  registers[v.index | 0x08] = latch.envx;
}
