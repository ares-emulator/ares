//returns VT[e]
auto RSP::vte() -> v128 {
  //SSE byte indexes are in little-endian ordering; read from right to left
  static const __m128i shuffle[16] = {
    //vector
    _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15),  //01234567
    _mm_setr_epi8( 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15),  //01234567
    //scalar quarter
    _mm_setr_epi8( 2, 3, 2, 3, 6, 7, 6, 7,10,11,10,11,14,15,14,15),  //00224466
    _mm_setr_epi8( 0, 1, 0, 1, 4, 5, 4, 5, 8, 9, 8, 9,12,13,12,13),  //11335577
    //scalar half
    _mm_setr_epi8( 6, 7, 6, 7, 6, 7, 6, 7,14,15,14,15,14,15,14,15),  //00004444
    _mm_setr_epi8( 4, 5, 4, 5, 4, 5, 4, 5,12,13,12,13,12,13,12,13),  //11115555
    _mm_setr_epi8( 2, 3, 2, 3, 2, 3, 2, 3,10,11,10,11,10,11,10,11),  //22226666
    _mm_setr_epi8( 0, 1, 0, 1, 0, 1, 0, 1, 8, 9, 8, 9, 8, 9, 8, 9),  //33337777
    //scalar whole
    _mm_setr_epi8(14,15,14,15,14,15,14,15,14,15,14,15,14,15,14,15),  //00000000
    _mm_setr_epi8(12,13,12,13,12,13,12,13,12,13,12,13,12,13,12,13),  //11111111
    _mm_setr_epi8(10,11,10,11,10,11,10,11,10,11,10,11,10,11,10,11),  //22222222
    _mm_setr_epi8( 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9),  //33333333
    _mm_setr_epi8( 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7),  //44444444
    _mm_setr_epi8( 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5),  //55555555
    _mm_setr_epi8( 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3),  //66666666
    _mm_setr_epi8( 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1),  //77777777
  };
  //todo: benchmark to see if testing for cases 0&1 to return value directly is faster
  return {u128(_mm_shuffle_epi8(VT.s128, shuffle[OP >> 21 & 15]))};
}

auto RSP::instructionCFC2() -> void {
  v128 hi, lo;
  switch(RDn & 3) {
  case 0x00: hi = VCOH; lo = VCOL; break;
  case 0x01: hi = VCCH; lo = VCCL; break;
  case 0x02: hi = zero; lo = VCE;  break;
  case 0x03: hi = zero; lo = VCE;  break;  //unverified
  }

  u16 value = _mm_movemask_epi8(_mm_packs_epi16(hi, lo));
  value = (value & 0xff00) >> 8 | (value & 0x00ff) << 8;
  value = (value & 0xf0f0) >> 4 | (value & 0x0f0f) << 4;
  value = (value & 0xcccc) >> 2 | (value & 0x3333) << 2;
  value = (value & 0xaaaa) >> 1 | (value & 0x5555) << 1;
  RT.u32 = i16(value);
}

auto RSP::instructionCTC2() -> void {
  maybe<v128&> hi, lo;
  v128 zero;
  switch(RDn & 3) {
  case 0x00: hi = VCOH; lo = VCOL; break;
  case 0x01: hi = VCCH; lo = VCCL; break;
  case 0x02: hi = zero; lo = VCE;  break;
  case 0x03: hi = zero; lo = VCE;  break;  //unverified
  }

  #if 1
  u16 rt = RT.u32;
  lo->s128 = _mm_set_epi16(
    0 - (rt >>  0 & 1), 0 - (rt >>  1 & 1), 0 - (rt >>  2 & 1), 0 - (rt >>  3 & 1),
    0 - (rt >>  4 & 1), 0 - (rt >>  5 & 1), 0 - (rt >>  6 & 1), 0 - (rt >>  7 & 1)
  );
  hi->s128 = _mm_set_epi16(
    0 - (rt >>  8 & 1), 0 - (rt >>  9 & 1), 0 - (rt >> 10 & 1), 0 - (rt >> 11 & 1),
    0 - (rt >> 12 & 1), 0 - (rt >> 13 & 1), 0 - (rt >> 14 & 1), 0 - (rt >> 15 & 1)
  );
  #endif

  #if 0
  u16 rt = RT.u32;
  for(uint bit : range(8)) {
    hi->element(bit) = 0 - bool(rt & 0x0100 << bit);
    lo->element(bit) = 0 - bool(rt & 0x0001 << bit);
  }
  #endif
}

auto RSP::instructionLBV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP);
  auto index = OP >> 7 & 15;
  data.byte(index) = dmem.readByte(address);
}

auto RSP::instructionLDV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 8;
  auto start = OP >> 7 & 15;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    data.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLFV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  auto start = index >> 1;
  auto end = start + 4;
  for(uint offset = start; offset < end; offset++) {
    data.element(offset & 7) = dmem.readByte(address) << 7;
    address += 4;
  }
}

auto RSP::instructionLHV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  for(uint offset = 0; offset < 8; offset++) {
    data.element(offset) = dmem.readByte(address + (16 - index + offset * 2 & 15)) << 7;
  }
}

auto RSP::instructionLLV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 4;
  auto start = OP >> 7 & 15;
  auto end = start + 4;
  for(uint offset = start; offset < end; offset++) {
    data.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLPV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 8;
  auto index = OP >> 7 & 15;
  for(uint offset = 0; offset < 8; offset++) {
    data.element(offset) = dmem.readByte(address + (16 - index + offset & 15)) << 8;
  }
}

auto RSP::instructionLQV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto start = 0;  //OP >> 7 & 15; ?
  auto end = 16 - (address & 15);
  for(uint offset = start; offset < end; offset++) {
    data.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLRV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;  //0?
  auto start = 16 - ((address & 15) - index);
  address &= ~15;
  for(uint offset = start; offset < 16; offset++) {
    data.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLSV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 2;
  auto start = OP >> 7 & 15;
  auto end = start + 2;
  for(uint offset = start; offset < end; offset++) {
    data.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLTV() -> void {
  #if 1
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  auto start = OP >> 16 & 31;
  auto end = min(32, start + 8);
  address = (address + 8 & ~15) + (index & 1);
  for(uint offset = start; offset < end; offset++) {
    auto byte = (8 - (index >> 1) + (offset - start)) << 1;
    VPR[offset].byte(byte + 0 & 15) = dmem.readByte(address++);
    VPR[offset].byte(byte + 1 & 15) = dmem.readByte(address++);
  }
  #endif

  #if 0
   u32 address = RS.u32 + int7(OP) * 16;
   u32 element = OP >> 8 & 7;
   u32 vt = VTn & ~7;
  v128 vs{dmem.readQuadUnaligned(address)};
  for(u32 index : range(8)) {
    VPR[vt++].element(index - element & 7) = vs.element(index);
  }
  #endif
}

auto RSP::instructionLUV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 8;
  auto index = OP >> 7 & 15;
  for(uint offset = 0; offset < 8; offset++) {
    data.element(offset) = dmem.readByte(address + (16 - index + offset & 15)) << 7;
  }
}

auto RSP::instructionLWC2() -> void {
  switch(OP >> 11 & 31) {
  case 0x00: return instructionLBV();
  case 0x01: return instructionLSV();
  case 0x02: return instructionLLV();
  case 0x03: return instructionLDV();
  case 0x04: return instructionLQV();
  case 0x05: return instructionLRV();
  case 0x06: return instructionLPV();
  case 0x07: return instructionLUV();
  case 0x08: return instructionLHV();
  case 0x09: return instructionLFV();
//case 0x0a: return instructionLWV();  //not present on N64 RSP
  case 0x0b: return instructionLTV();
  }
}

auto RSP::instructionLWV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  auto start = 16 - index;
  auto end = index + 16;
  for(uint offset = start; offset < end; offset++) {
    data.byte(offset & 15) = dmem.readByte(address);
    address += 4;
  }
}

auto RSP::instructionMFC2() -> void {
  auto index = OP >> 7 & 15;
  auto hi = VS.byte(index + 0 & 15);
  auto lo = VS.byte(index + 1 & 15);
  RT.u32 = i16(hi << 8 | lo << 0);
}

auto RSP::instructionMTC2() -> void {
  auto index = OP >> 7 & 15;
  VS.byte(index + 0 & 15) = RT.u32 >> 8;
  VS.byte(index + 1 & 15) = RT.u32 >> 0;
}

auto RSP::instructionSBV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP);
  auto index = OP >> 7 & 15;
  dmem.writeByte(address, data.byte(index));
}

auto RSP::instructionSDV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 8;
  auto start = OP >> 7 & 15;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, data.byte(offset & 15));
  }
}

auto RSP::instructionSFV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  auto start = index >> 1;
  auto end = start + 4;
  auto base = address & 15;
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address + (base & 15), data.element(offset & 7) >> 7);
    base += 4;
  }
}

auto RSP::instructionSHV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  for(uint offset = 0; offset < 8; offset++) {
    auto byte = index + offset * 2;
    auto value = data.byte(byte + 0 & 15) << 1 | data.byte(byte + 1 & 15) >> 7;
    dmem.writeByte(address, value);
    address += 2;
  }
}

auto RSP::instructionSLV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 4;
  auto start = OP >> 7 & 15;
  auto end = start + 4;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, data.byte(offset & 15));
  }
}

auto RSP::instructionSPV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 8;
  auto start = OP >> 7 & 15;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    if((offset & 15) < 8) {
      dmem.writeByte(address++, data.byte((offset & 7) << 1));
    } else {
      dmem.writeByte(address++, data.element(offset & 7) >> 7);
    }
  }
}

auto RSP::instructionSQV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto start = OP >> 7 & 15;
  auto end = start + (16 - (address & 15));
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, data.byte(offset & 15));
  }
}

auto RSP::instructionSRV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto start = OP >> 7 & 15;
  auto end = start + (address & 15);
  auto base = 16 - (address & 15);
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, data.byte(offset + base & 15));
  }
}

auto RSP::instructionSSV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 2;
  auto start = OP >> 7 & 15;
  auto end = start + 2;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, data.byte(offset & 15));
  }
}

auto RSP::instructionSTV() -> void {
  #if 1
  auto address = RS.u32 + int7(OP) * 16;
  auto index = OP >> 7 & 15;
  auto start = OP >> 16 & 31;
  auto end = min(32, start + 8);
  auto element = 8 - (index >> 1);
  auto base = (address & 15) + (element << 1);
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeHalfUnaligned(address + (base & 15), VPR[offset].element(element++ & 7));
    base += 2;
  }
  #endif

  #if 0
   u32 address = RS.u32 + int7(OP) * 16;
   u32 element = OP >> 8 & 7;
   u32 vt = VTn & ~7;
  v128 vs;
  for(u32 index : range(8)) {
    vs.element(index - element & 7) = VPR[vt++].element(index - element & 7);
  }
  dmem.writeQuadUnaligned(address, vs.u128);
  #endif
}

auto RSP::instructionSUV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 8;
  auto start = OP >> 7 & 15;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    if((offset & 15) < 8) {
      dmem.writeByte(address++, data.element(offset & 7) >> 7);
    } else {
      dmem.writeByte(address++, data.byte((offset & 7) << 1));
    }
  }
}

auto RSP::instructionSWC2() -> void {
  switch(OP >> 11 & 31) {
  case 0x00: return instructionSBV();
  case 0x01: return instructionSSV();
  case 0x02: return instructionSLV();
  case 0x03: return instructionSDV();
  case 0x04: return instructionSQV();
  case 0x05: return instructionSRV();
  case 0x06: return instructionSPV();
  case 0x07: return instructionSUV();
  case 0x08: return instructionSHV();
  case 0x09: return instructionSFV();
  case 0x0a: return instructionSWV();
  case 0x0b: return instructionSTV();
  }
}

auto RSP::instructionSWV() -> void {
  auto& data = VT;
  auto address = RS.u32 + int7(OP) * 16;
  auto start = OP >> 7 & 15;
  auto end = start + 16;
  auto base = address & 15;
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address + (base++ & 15), data.byte(offset & 15));
  }

  #if 0
   u32 address = RS.u32 + int7(OP) * 16;
   u32 rotate = (OP >> 8 & 7) * 16;
  v128 vt{VPR[VTn].u128};
  v128 vs{vt.u128 << rotate | vt.u128 >> 128 - rotate};
  dmem.writeQuadUnaligned(address, vs.u128);
  #endif
}

auto RSP::instructionVABS() -> void {
  v128 vte = VTe, vs0, slt;
  vs0  = _mm_cmpeq_epi16(VS, zero);
  slt  = _mm_srai_epi16(VS, 15);
  VD   = _mm_andnot_si128(vs0, vte);
  VD   = _mm_xor_si128(VD, slt);
  ACCL = _mm_sub_epi16(VD, slt);
  VD   = _mm_subs_epi16(VD, slt);
}

auto RSP::instructionVADD() -> void {
  v128 vte = VTe, sum, min, max;
  sum  = _mm_add_epi16(VS, vte);
  ACCL = _mm_sub_epi16(sum, VCOL);
  min  = _mm_min_epi16(VS, vte);
  max  = _mm_max_epi16(VS, vte);
  min  = _mm_subs_epi16(min, VCOL);
  VD   = _mm_adds_epi16(min, max);
  VCOL = zero;
  VCOH = zero;
}

auto RSP::instructionVADDC() -> void {
  v128 vte = VTe, sum;
  sum  = _mm_adds_epu16(VS, vte);
  ACCL = _mm_add_epi16(VS, vte);
  VCOL = _mm_cmpeq_epi16(sum, ACCL);
  VCOL = _mm_cmpeq_epi16(VCOL, zero);
  VCOH = zero;
  VD   = ACCL;
}

auto RSP::instructionVAND() -> void {
  ACCL = _mm_and_si128(VS, VTe);
  VD   = ACCL;
}

auto RSP::instructionVCL() -> void {
  v128 vte = VTe, nvt, diff, ncarry, nvce, diff0, lec1, lec2, leeq, geeq, le, ge, mask;
  nvt    = _mm_xor_si128(vte, VCOL);
  nvt    = _mm_sub_epi16(nvt, VCOL);
  diff   = _mm_sub_epi16(VS, nvt);
  ncarry = _mm_adds_epu16(VS, vte);
  ncarry = _mm_cmpeq_epi16(diff, ncarry);
  nvce   = _mm_cmpeq_epi16(VCE, zero);
  diff0  = _mm_cmpeq_epi16(diff, zero);
  lec1   = _mm_and_si128(diff0, ncarry);
  lec1   = _mm_and_si128(nvce, lec1);
  lec2   = _mm_or_si128(diff0, ncarry);
  lec2   = _mm_and_si128(VCE, lec2);
  leeq   = _mm_or_si128(lec1, lec2);
  geeq   = _mm_subs_epu16(vte, VS);
  geeq   = _mm_cmpeq_epi16(geeq, zero);
  le     = _mm_andnot_si128(VCOH, VCOL);
  le     = _mm_blendv_epi8(VCCL, leeq, le);
  ge     = _mm_or_si128(VCOL, VCOH);
  ge     = _mm_blendv_epi8(geeq, VCCH, ge);
  mask   = _mm_blendv_epi8(ge, le, VCOL);
  ACCL   = _mm_blendv_epi8(VS, nvt, mask);
  VCCH   = ge;
  VCCL   = le;
  VCOH   = zero;
  VCOL   = zero;
  VCE    = zero;
  VD     = ACCL;
}

auto RSP::instructionVCH() -> void {
  v128 vte = VTe, nvt, diff, diff0, vtn, dlez, dgez, mask;
  VCOL  = _mm_xor_si128(VS, vte);
  VCOL  = _mm_cmplt_epi16(VCOL, zero);
  nvt   = _mm_xor_si128(vte, VCOL);
  nvt   = _mm_sub_epi16(nvt, VCOL);
  diff  = _mm_sub_epi16(VS, nvt);
  diff0 = _mm_cmpeq_epi16(diff, zero);
  vtn   = _mm_cmplt_epi16(vte, zero);
  dlez  = _mm_cmpgt_epi16(diff, zero);
  dgez  = _mm_or_si128(dlez, diff0);
  dlez  = _mm_cmpeq_epi16(zero, dlez);
  VCCH  = _mm_blendv_epi8(dgez, vtn, VCOL);
  VCCL  = _mm_blendv_epi8(vtn, dlez, VCOL);
  VCE   = _mm_cmpeq_epi16(diff, VCOL);
  VCE   = _mm_and_si128(VCE, VCOL);
  VCOH  = _mm_or_si128(diff0, VCE);
  VCOH  = _mm_cmpeq_epi16(VCOH, zero);
  mask  = _mm_blendv_epi8(VCCH, VCCL, VCOL);
  ACCL  = _mm_blendv_epi8(VS, nvt, mask);
  VD    = ACCL;
}

auto RSP::instructionVCR() -> void {
  v128 vte = VTe, sign, dlez, dgez, nvt, mask;
  sign = _mm_xor_si128(VS, vte);
  sign = _mm_srai_epi16(sign, 15);
  dlez = _mm_and_si128(VS, sign);
  dlez = _mm_add_epi16(dlez, vte);
  VCCL = _mm_srai_epi16(dlez, 15);
  dgez = _mm_or_si128(VS, sign);
  dgez = _mm_min_epi16(dgez, vte);
  VCCH = _mm_cmpeq_epi16(dgez, vte);
  nvt  = _mm_xor_si128(vte, sign);
  mask = _mm_blendv_epi8(VCCH, VCCL, sign);
  ACCL = _mm_blendv_epi8(VS, nvt, mask);
  VD   = ACCL;
  VCOL = zero;
  VCOH = zero;
  VCE  = zero;
}

auto RSP::instructionVEQ() -> void {
  v128 vte = VTe, eq;
  eq   = _mm_cmpeq_epi16(VS, vte);
  VCCL = _mm_andnot_si128(VCOH, eq);
  ACCL = _mm_blendv_epi8(vte, VS, VCCL);
  VCCH = zero;
  VCOH = zero;
  VCOL = zero;
  VD   = ACCL;
}

auto RSP::instructionVGE() -> void {
  v128 vte = VTe, eq, gt, es;
  eq   = _mm_cmpeq_epi16(VS, vte);
  gt   = _mm_cmpgt_epi16(VS, vte);
  es   = _mm_and_si128(VCOH, VCOL);
  eq   = _mm_andnot_si128(es, eq);
  VCCL = _mm_or_si128(gt, eq);
  ACCL = _mm_blendv_epi8(vte, VS, VCCL);
  VCCH = zero;
  VCOH = zero;
  VCOL = zero;
  VD   = ACCL;
}

auto RSP::instructionVLT() -> void {
  v128 vte = VTe, eq, lt;
  eq   = _mm_cmpeq_epi16(VS, vte);
  lt   = _mm_cmplt_epi16(VS, vte);
  eq   = _mm_and_si128(VCOH, eq);
  eq   = _mm_and_si128(VCOL, eq);
  VCCL = _mm_or_si128(lt, eq);
  ACCL = _mm_blendv_epi8(vte, VS, VCCL);
  VCCH = zero;
  VCOH = zero;
  VCOL = zero;
  VD   = ACCL;
}

auto RSP::instructionVMACF(bool U) -> void {
  v128 vte = VTe, lo, md, hi, carry, omask;
  lo    = _mm_mullo_epi16(VS, vte);
  hi    = _mm_mulhi_epi16(VS, vte);
  md    = _mm_slli_epi16(hi, 1);
  carry = _mm_srli_epi16(lo, 15);
  hi    = _mm_srai_epi16(hi, 15);
  md    = _mm_or_si128(md, carry);
  lo    = _mm_slli_epi16(lo, 1);
  omask = _mm_adds_epu16(ACCL, lo);
  ACCL  = _mm_add_epi16(ACCL, lo);
  omask = _mm_cmpeq_epi16(ACCL, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  md    = _mm_sub_epi16(md, omask);
  carry = _mm_cmpeq_epi16(md, zero);
  carry = _mm_and_si128(carry, omask);
  hi    = _mm_sub_epi16(hi, carry);
  omask = _mm_adds_epu16(ACCM, md);
  ACCM  = _mm_add_epi16(ACCM, md);
  omask = _mm_cmpeq_epi16(ACCM, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  ACCH  = _mm_add_epi16(ACCH, hi);
  ACCH  = _mm_sub_epi16(ACCH, omask);
  if(!U) {
    lo = _mm_unpacklo_epi16(ACCM, ACCH);
    hi = _mm_unpackhi_epi16(ACCM, ACCH);
    VD = _mm_packs_epi32(lo, hi);
  } else {
    v128 mmask, hmask;
    mmask = _mm_srai_epi16(ACCM, 15);
    hmask = _mm_srai_epi16(ACCH, 15);
    md    = _mm_or_si128(mmask, ACCM);
    omask = _mm_cmpgt_epi16(ACCH, zero);
    md    = _mm_andnot_si128(hmask, md);
    VD    = _mm_or_si128(omask, md);
  }
}

auto RSP::instructionVMACQ() -> void {
  for(uint n : range(8)) {
    i32 product = ACCH.element(n) << 16 | ACCM.element(n) << 0;
    if(product < 0 && !(product & 1 << 5)) product += 32;
    else if(product >= 0 && !(product & 1 << 5)) product -= 32;
    ACCH.element(n) = product >> 16;
    ACCM.element(n) = product >>  0;
    ACCL.element(n) = 0;  //unconfirmed
    VD.element(n) = sclamp<16>(product >> 1) & ~15;
  }
}

auto RSP::instructionVMADH() -> void {
  v128 vte = VTe, lo, hi, omask;
  lo    = _mm_mullo_epi16(VS, vte);
  hi    = _mm_mulhi_epi16(VS, vte);
  omask = _mm_adds_epu16(ACCM, lo);
  ACCM  = _mm_add_epi16(ACCM, lo);
  omask = _mm_cmpeq_epi16(ACCM, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  hi    = _mm_sub_epi16(hi, omask);
  ACCH  = _mm_add_epi16(ACCH, hi);
  lo    = _mm_unpacklo_epi16(ACCM, ACCH);
  hi    = _mm_unpackhi_epi16(ACCM, ACCH);
  VD    = _mm_packs_epi32(lo, hi);
}

auto RSP::instructionVMADL() -> void {
  v128 vte = VTe, hi, omask, nhi, nmd, shi, smd, cmask, cval;
  hi    = _mm_mulhi_epu16(VS, vte);
  omask = _mm_adds_epu16(ACCL, hi);
  ACCL  = _mm_add_epi16(ACCL, hi);
  omask = _mm_cmpeq_epi16(ACCL, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  hi    = _mm_sub_epi16(zero, omask);
  omask = _mm_adds_epu16(ACCM, hi);
  ACCM  = _mm_add_epi16(ACCM, hi);
  omask = _mm_cmpeq_epi16(ACCM, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  ACCH  = _mm_sub_epi16(ACCH, omask);
  nhi   = _mm_srai_epi16(ACCH, 15);
  nmd   = _mm_srai_epi16(ACCM, 15);
  shi   = _mm_cmpeq_epi16(nhi, ACCH);
  smd   = _mm_cmpeq_epi16(nhi, nmd);
  cmask = _mm_and_si128(smd, shi);
  cval  = _mm_cmpeq_epi16(nhi, zero);
  VD    = _mm_blendv_epi8(cval, ACCL, cmask);
}

auto RSP::instructionVMADM() -> void {
  v128 vte = VTe, lo, hi, sign, vt, omask;
  lo    = _mm_mullo_epi16(VS, vte);
  hi    = _mm_mulhi_epu16(VS, vte);
  sign  = _mm_srai_epi16(VS, 15);
  vt    = _mm_and_si128(vte, sign);
  hi    = _mm_sub_epi16(hi, vt);
  omask = _mm_adds_epu16(ACCL, lo);
  ACCL  = _mm_add_epi16(ACCL, lo);
  omask = _mm_cmpeq_epi16(ACCL, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  hi    = _mm_sub_epi16(hi, omask);
  omask = _mm_adds_epu16(ACCM, hi);
  ACCM  = _mm_add_epi16(ACCM, hi);
  omask = _mm_cmpeq_epi16(ACCM, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  hi    = _mm_srai_epi16(hi, 15);
  ACCH  = _mm_add_epi16(ACCH, hi);
  ACCH  = _mm_sub_epi16(ACCH, omask);
  lo    = _mm_unpacklo_epi16(ACCM, ACCH);
  hi    = _mm_unpackhi_epi16(ACCM, ACCH);
  VD    = _mm_packs_epi32(lo, hi);
}

auto RSP::instructionVMADN() -> void {
  v128 vte = VTe, lo, hi, sign, vs, omask, nhi, nmd, shi, smd, cmask, cval;
  lo    = _mm_mullo_epi16(VS, vte);
  hi    = _mm_mulhi_epu16(VS, vte);
  sign  = _mm_srai_epi16(vte, 15);
  vs    = _mm_and_si128(VS, sign);
  hi    = _mm_sub_epi16(hi, vs);
  omask = _mm_adds_epu16(ACCL, lo);
  ACCL  = _mm_add_epi16(ACCL, lo);
  omask = _mm_cmpeq_epi16(ACCL, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  hi    = _mm_sub_epi16(hi, omask);
  omask = _mm_adds_epu16(ACCM, hi);
  ACCM  = _mm_add_epi16(ACCM, hi);
  omask = _mm_cmpeq_epi16(ACCM, omask);
  omask = _mm_cmpeq_epi16(omask, zero);
  hi    = _mm_srai_epi16(hi, 15);
  ACCH  = _mm_add_epi16(ACCH, hi);
  ACCH  = _mm_sub_epi16(ACCH, omask);
  nhi   = _mm_srai_epi16(ACCH, 15);
  nmd   = _mm_srai_epi16(ACCM, 15);
  shi   = _mm_cmpeq_epi16(nhi, ACCH);
  smd   = _mm_cmpeq_epi16(nhi, nmd);
  cmask = _mm_and_si128(smd, shi);
  cval  = _mm_cmpeq_epi16(nhi, zero);
  VD    = _mm_blendv_epi8(cval, ACCL, cmask);
}

auto RSP::instructionVMOV() -> void {
  u32 de = OP >> 11 & 7;
  ACCL = vte();
  VD.element(de) = ACCL.element(de);
}

auto RSP::instructionVMRG() -> void {
  ACCL = _mm_blendv_epi8(VTe, VS, VCCL);
  VCOH = zero;
  VCOL = zero;
  VD   = ACCL;
}

auto RSP::instructionVMUDH() -> void {
  v128 vte = VTe, lo, hi;
  ACCL = zero;
  ACCM = _mm_mullo_epi16(VS, vte);
  ACCH = _mm_mulhi_epi16(VS, vte);
  lo   = _mm_unpacklo_epi16(ACCM, ACCH);
  hi   = _mm_unpackhi_epi16(ACCM, ACCH);
  VD   = _mm_packs_epi32(lo, hi);
}

auto RSP::instructionVMUDL() -> void {
  ACCL = _mm_mulhi_epu16(VS, VTe);
  ACCM = zero;
  ACCH = zero;
  VD   = ACCL;
}

auto RSP::instructionVMUDM() -> void {
  v128 vte = VTe, sign, vt;
  ACCL = _mm_mullo_epi16(VS, vte);
  ACCM = _mm_mulhi_epu16(VS, vte);
  sign = _mm_srai_epi16(VS, 15);
  vt   = _mm_and_si128(vte, sign);
  ACCM = _mm_sub_epi16(ACCM, vt);
  ACCH = _mm_srai_epi16(ACCM, 15);
  VD   = ACCM;
}

auto RSP::instructionVMUDN() -> void {
  v128 vte = VTe, sign, vs;
  ACCL = _mm_mullo_epi16(VS, vte);
  ACCM = _mm_mulhi_epu16(VS, vte);
  sign = _mm_srai_epi16(vte, 15);
  vs   = _mm_and_si128(VS, sign);
  ACCM = _mm_sub_epi16(ACCM, vs);
  ACCH = _mm_srai_epi16(ACCM, 15);
  VD   = ACCL;
}

auto RSP::instructionVMULF(bool U) -> void {
  v128 vte = VTe, lo, hi, round, sign1, sign2, neq, eq, neg;
  lo    = _mm_mullo_epi16(VS, vte);
  round = _mm_cmpeq_epi16(zero, zero);
  sign1 = _mm_srli_epi16(lo, 15);
  lo    = _mm_add_epi16(lo, lo);
  round = _mm_slli_epi16(round, 15);
  hi    = _mm_mulhi_epi16(VS, vte);
  sign2 = _mm_srli_epi16(lo, 15);
  ACCL  = _mm_add_epi16(round, lo);
  sign1 = _mm_add_epi16(sign1, sign2);
  hi    = _mm_slli_epi16(hi, 1);
  neq   = _mm_cmpeq_epi16(VS, vte);
  ACCM  = _mm_add_epi16(hi, sign1);
  neg   = _mm_srai_epi16(ACCM, 15);
  if(!U) {
    eq   = _mm_and_si128(neq, neg);
    ACCH = _mm_andnot_si128(neq, neg);
    VD   = _mm_add_epi16(ACCM, eq);
  } else {
    ACCH = _mm_andnot_si128(neq, neg);
    hi   = _mm_or_si128(ACCM, neg);
    VD   = _mm_andnot_si128(ACCH, hi);
  }
}

auto RSP::instructionVMULQ() -> void {
  v128 vte = VTe;
  for(uint n : range(8)) {
    i32 product = (i16)VS.element(n) * (i16)vte.element(n);
    if(product < 0) product += 31;  //round
    ACCH.element(n) = product >> 16;
    ACCM.element(n) = product >>  0;
    ACCL.element(n) = 0;  //unconfirmed
    VD.element(n) = sclamp<16>(product >> 1) & ~15;
  }
}

auto RSP::instructionVNAND() -> void {
  ACCL = _mm_and_si128(VS, VTe);
  ACCL = _mm_xor_si128(ACCL, invert);
  VD   = ACCL;
}

auto RSP::instructionVNE() -> void {
  v128 vte = VTe, eq, ne;
  eq   = _mm_cmpeq_epi16(VS, vte);
  ne   = _mm_cmpeq_epi16(eq, zero);
  VCCL = _mm_and_si128(VCOH, eq);
  VCCL = _mm_or_si128(VCCL, ne);
  ACCL = _mm_blendv_epi8(vte, VS, VCCL);
  VCCH = zero;
  VCOH = zero;
  VCOL = zero;
  VD   = ACCL;
}

auto RSP::instructionVNOP() -> void {
}

auto RSP::instructionVNOR() -> void {
  ACCL = _mm_or_si128(VS, VTe);
  ACCL = _mm_xor_si128(ACCL, invert);
  VD   = ACCL;
}

auto RSP::instructionVNXOR() -> void {
  ACCL = _mm_xor_si128(VS, VTe);
  ACCL = _mm_xor_si128(ACCL, invert);
  VD   = ACCL;
}

auto RSP::instructionVOR() -> void {
  ACCL = _mm_or_si128(VS, VTe);
  VD   = ACCL;
}

auto RSP::instructionVRCP(bool L) -> void {
  i32 result = 0;
  i32 input = L && DIVDP ? DIVIN << 16 | VT.element(En & 7) : i16(VT.element(En & 7));
  i32 mask = input >> 31;
  i32 data = input ^ mask;
  if(input > -32768) data -= mask;
  if(data == 0) {
    result = 0x7fff'ffff;
  } else if(input == -32768) {
    result = 0xffff'0000;
  } else {
    u32 shift = __builtin_clz(data);
    u32 index = (u64(data) << shift & 0x7fc0'0000) >> 22;
    result = reciprocals[index];
    result = (0x10000 | result) << 14;
    result = result >> 31 - shift ^ mask;
  }
  DIVDP = 0;
  DIVOUT = result >> 16;
  ACCL = VTe;
  VD.element(DEn & 7) = result;
}

auto RSP::instructionVRCPH() -> void {
  ACCL  = VTe;
  DIVDP = 1;
  DIVIN = VT.element(En & 7);
  VD.element(DEn & 7) = DIVOUT;
}

auto RSP::instructionVRND(bool D) -> void {
  v128 vte = VTe;
  for(uint n : range(8)) {
    i32 product = (i16)vte.element(n);
    if(VSn & 1) product <<= 16;
    i64 acc = 0;
    acc |= ACCH.element(n); acc <<= 16;
    acc |= ACCM.element(n); acc <<= 16;
    acc |= ACCL.element(n); acc <<= 16;
    acc >>= 16;
    if(D == 0 && acc <  0) acc += product;
    if(D == 1 && acc >= 0) acc += product;
    ACCH.element(n) = acc >> 32;
    ACCM.element(n) = acc >> 16;
    ACCL.element(n) = acc >>  0;
    VD.element(n) = acc >> 16;
  }
}

auto RSP::instructionVRSQ(bool L) -> void {
  i32 result = 0;
  i32 input = L && DIVDP ? DIVIN << 16 | VT.element(En & 7) : i16(VT.element(En & 7));
  i32 mask = input >> 31;
  i32 data = input ^ mask;
  if(input > -32768) data -= mask;
  if(data == 0) {
    result = 0x7fff'ffff;
  } else if(input == -32768) {
    result = 0xffff'0000;
  } else {
    u32 shift = __builtin_clz(data);
    u32 index = (u64(data) << shift & 0x7fc0'0000) >> 22;
    result = inverseSquareRoots[index & 0x1fe | shift & 1];
    result = (0x10000 | result) << 14;
    result = result >> (31 - shift >> 1) ^ mask;
  }
  DIVDP = 0;
  DIVOUT = result >> 16;
  ACCL = VTe;
  VD.element(DEn & 7) = result;
}

auto RSP::instructionVRSQH() -> void {
  ACCL  = VTe;
  DIVDP = 1;
  DIVIN = VT.element(En & 7);
  VD.element(DEn & 7) = DIVOUT;
}

auto RSP::instructionVSAR() -> void {
  switch(En) {
  case 0x8: VD = ACCH; break;
  case 0x9: VD = ACCM; break;
  case 0xa: VD = ACCL; break;
  default:  VD = zero; break;
  }
}

auto RSP::instructionVSUB() -> void {
  v128 vte = VTe, udiff, sdiff, ov;
  udiff = _mm_sub_epi16(vte, VCOL);
  sdiff = _mm_subs_epi16(vte, VCOL);
  ACCL  = _mm_sub_epi16(VS, udiff);
  ov    = _mm_cmpgt_epi16(sdiff, udiff);
  VD    = _mm_subs_epi16(VS, sdiff);
  VD    = _mm_adds_epi16(VD, ov);
  VCOL  = zero;
  VCOH  = zero;
}

auto RSP::instructionVSUBC() -> void {
  v128 vte = VTe, equal, udiff, diff0;
  udiff = _mm_subs_epu16(VS, vte);
  equal = _mm_cmpeq_epi16(VS, vte);
  diff0 = _mm_cmpeq_epi16(udiff, zero);
  VCOH  = _mm_cmpeq_epi16(equal, zero);
  VCOL  = _mm_andnot_si128(equal, diff0);
  ACCL  = _mm_sub_epi16(VS, vte);
  VD    = ACCL;
}

auto RSP::instructionVXOR() -> void {
  ACCL = _mm_xor_si128(VS, VTe);
  VD   = ACCL;
}
