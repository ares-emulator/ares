auto SPU::Reverb::process(i16 linput, i16 rinput) -> std::pair<i32, i32> {
  lastInput[0] = downsampleBuffer[0][resamplePosition | 0x00] = downsampleBuffer[0][resamplePosition | 0x40] = linput;
  lastInput[1] = downsampleBuffer[1][resamplePosition | 0x00] = downsampleBuffer[1][resamplePosition | 0x40] = rinput;

  i32 loutput = 0;
  i32 routput = 0;
  if(resamplePosition & 1) {
    compute();
    loutput = r2244<1>(&upsampleBuffer[0][(resamplePosition - 39 & 0x3f) >> 1]);
    routput = r2244<1>(&upsampleBuffer[1][(resamplePosition - 39 & 0x3f) >> 1]);
  } else {
    loutput = r2244<0>(&upsampleBuffer[0][(resamplePosition - 39 & 0x3f) >> 1]);
    routput = r2244<0>(&upsampleBuffer[1][(resamplePosition - 39 & 0x3f) >> 1]);
  }
  resamplePosition++;

  lastOutput[0] = loutput = amplify(loutput, vLOUT);
  lastOutput[1] = routput = amplify(routput, vROUT);
  return {loutput, routput};
}

auto SPU::Reverb::compute() -> void {
  i32 ldownsample = r4422(&downsampleBuffer[0][(resamplePosition - 39) & 0x3f]);
  i32 rdownsample = r4422(&downsampleBuffer[1][(resamplePosition - 39) & 0x3f]);

  if(enable) {
    i16 IIR_INPUT_A0 = sclamp<16>((read(IIR_SRC_A0) * IIR_COEF >> 15) + (ldownsample * IN_COEF_L >> 15));
    i16 IIR_INPUT_A1 = sclamp<16>((read(IIR_SRC_A1) * IIR_COEF >> 15) + (rdownsample * IN_COEF_R >> 15));
    i16 IIR_INPUT_B0 = sclamp<16>((read(IIR_SRC_B0) * IIR_COEF >> 15) + (ldownsample * IN_COEF_L >> 15));
    i16 IIR_INPUT_B1 = sclamp<16>((read(IIR_SRC_B1) * IIR_COEF >> 15) + (rdownsample * IN_COEF_R >> 15));

    i16 IIR_A0 = sclamp<16>(((IIR_INPUT_A0 * IIR_ALPHA >> 14) + (iiasm(read(IIR_DEST_A0, -1)) >> 14)) >> 1);
    i16 IIR_A1 = sclamp<16>(((IIR_INPUT_A1 * IIR_ALPHA >> 14) + (iiasm(read(IIR_DEST_A1, -1)) >> 14)) >> 1);
    i16 IIR_B0 = sclamp<16>(((IIR_INPUT_B0 * IIR_ALPHA >> 14) + (iiasm(read(IIR_DEST_B0, -1)) >> 14)) >> 1);
    i16 IIR_B1 = sclamp<16>(((IIR_INPUT_B1 * IIR_ALPHA >> 14) + (iiasm(read(IIR_DEST_B1, -1)) >> 14)) >> 1);

    write(IIR_DEST_A0, IIR_A0);
    write(IIR_DEST_A1, IIR_A1);
    write(IIR_DEST_B0, IIR_B0);
    write(IIR_DEST_B1, IIR_B1);

    i16 ACC0 = sclamp<16>(((read(ACC_SRC_A0) * ACC_COEF_A >> 14) + (read(ACC_SRC_B0) * ACC_COEF_B >> 14) + (read(ACC_SRC_C0) * ACC_COEF_C >> 14) + (read(ACC_SRC_D0) * ACC_COEF_D >> 14)) >> 1);
    i16 ACC1 = sclamp<16>(((read(ACC_SRC_A1) * ACC_COEF_A >> 14) + (read(ACC_SRC_B1) * ACC_COEF_B >> 14) + (read(ACC_SRC_C1) * ACC_COEF_C >> 14) + (read(ACC_SRC_D1) * ACC_COEF_D >> 14)) >> 1);

    i16 FB_A0 = read(MIX_DEST_A0 - FB_SRC_A);
    i16 FB_A1 = read(MIX_DEST_A1 - FB_SRC_A);
    i16 FB_B0 = read(MIX_DEST_B0 - FB_SRC_B);
    i16 FB_B1 = read(MIX_DEST_B1 - FB_SRC_B);

    write(MIX_DEST_A0, sclamp<16>(ACC0 - (FB_A0 * FB_ALPHA >> 15)));
    write(MIX_DEST_A1, sclamp<16>(ACC1 - (FB_A1 * FB_ALPHA >> 15)));

    write(MIX_DEST_B0, sclamp<16>((FB_ALPHA * ACC0 >> 15) - (FB_A0 * i16(0x8000 ^ FB_ALPHA) >> 15) - (FB_B0 * FB_X >> 15)));
    write(MIX_DEST_B1, sclamp<16>((FB_ALPHA * ACC1 >> 15) - (FB_A1 * i16(0x8000 ^ FB_ALPHA) >> 15) - (FB_B1 * FB_X >> 15)));
  }

  upsampleBuffer[0][resamplePosition >> 1 | 0x00] = upsampleBuffer[0][resamplePosition >> 1 | 0x20] = read(MIX_DEST_A0) + read(MIX_DEST_B0) >> 1;
  upsampleBuffer[1][resamplePosition >> 1 | 0x00] = upsampleBuffer[1][resamplePosition >> 1 | 0x20] = read(MIX_DEST_A1) + read(MIX_DEST_B1) >> 1;

  if(++currentAddress == 0) {
    currentAddress = baseAddress;
  }
}

auto SPU::Reverb::iiasm(i16 sample) -> i32 {
  if(IIR_ALPHA == -32768) return sample == -32768 ? 0 : sample * -65536;
  return sample * (32768 - IIR_ALPHA);
}

template<bool phase>
auto SPU::Reverb::r2244(const int16* source) -> i32 {
  i32 output = 0;
  if(phase) {
    output = source[9];
  } else {
    for(u32 n : range(20)) output += coefficients[n] * source[n];
    output = sclamp<16>(output >> 14);
  }
  return output;
}

auto SPU::Reverb::r4422(const int16* source) -> i32 {
  i32 output = 0;
  for(u32 n : range(20)) output += coefficients[n] * source[n * 2];
  output += 0x4000 * source[19];
  return sclamp<16>(output >> 15);
}

auto SPU::Reverb::address(u32 address) -> u32 {
  u32 offset = currentAddress + (address & 0x3ffff);
  offset += baseAddress & i32(offset << 13) >> 31;
  return (offset & 0x3ffff) * 2;
}

auto SPU::Reverb::read(u32 virtualAddress, i32 offset) -> i16 {
  u32 realAddress = address((virtualAddress << 2) + offset);
  return spu.ram.readHalf(realAddress);
}

auto SPU::Reverb::write(u32 virtualAddress, i16 data) -> void {
  u32 realAddress = address(virtualAddress << 2);
  return spu.ram.writeHalf(realAddress, data);
}
