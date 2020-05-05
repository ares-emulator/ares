auto RSP::instructionCFC2() -> void {
  v128 vdh, vdl, vd;
  switch(RDn) {
  case 0x00: vdh = VCOH; vdl = VCOL; break;
  case 0x01: vdh = VCCH; vdl = VCCL; break;
  case 0x02: vdh = zero; vdl = VCE;  break;
  default:   vdh = zero; vdl = zero; break;
  }
  vd = _mm_packs_epi16(vdh, vdl);
  //VPR elements are in big-endian order
  //VCx bits are in little-endian order
  //reverse the bits before storing in RT
  vd = _mm_shuffle_epi32  (vd, _MM_SHUFFLE(0, 1, 2, 3));
  vd = _mm_shufflelo_epi16(vd, _MM_SHUFFLE(2, 3, 0, 1));
  vd = _mm_shufflehi_epi16(vd, _MM_SHUFFLE(2, 3, 0, 1));
  RT.u64 = i64(_mm_movemask_epi8(vd));
}

auto RSP::instructionCTC2() -> void {
  maybe<v128&> vth, vtl;
  v128 zero;
  switch(RDn) {
  case 0x00: vth = VCOH; vtl = VCOL; break;
  case 0x01: vth = VCCH; vtl = VCCL; break;
  case 0x02: vth = zero; vtl = VCE;  break;
  default:   vth = zero; vtl = zero; break;
  }
  u16 rd = RD.u64;
  for(uint bit : range(8)) {
    vth->element(7 - bit) = 0 - bool(rd & 0x0100 << bit);
    vtl->element(7 - bit) = 0 - bool(rd & 0x0001 << bit);
  }
}

auto RSP::instructionLQV() -> void {
  u32 address = RS.u64 + int7(OP) * 16;
  VT.u128 = dmem.readQuad(address);
}

auto RSP::instructionLTV() -> void {
   u32 address = RS.u64 + int7(OP) * 16;
   u32 element = OP >> 8 & 7;
   u32 vt = VTn & ~7;
  v128 vs{dmem.readQuad(address)};
  for(u32 index : range(8)) {
    VPR[vt++].element(index - element & 7) = vs.element(index);
  }
}

auto RSP::instructionLWC2() -> void {
  if(!STATUS_COP2) return exception(CoprocessorUnusable);
  switch(OP >> 11 & 31) {
  case 0x04: return instructionLQV();
  case 0x0b: return instructionLTV();
  }
}

auto RSP::instructionMFC2() -> void {
  //todo
}

auto RSP::instructionMTC2() -> void {
  //todo
}

auto RSP::instructionSQV() -> void {
  u32 address = RS.u64 + int7(OP) * 16;
  dmem.writeQuad(address, VT.u128);
}

auto RSP::instructionSTV() -> void {
   u32 address = RS.u64 + int7(OP) * 16;
   u32 element = OP >> 8 & 7;
   u32 vt = VTn & ~7;
  v128 vs;
  for(u32 index : range(8)) {
    vs.element(index - element & 7) = VPR[vt++].element(index - element & 7);
  }
  dmem.writeQuad(address, vs.u128);
}

auto RSP::instructionSWC2() -> void {
  if(!STATUS_COP2) return exception(CoprocessorUnusable);
  switch(OP >> 11 & 31) {
  case 0x04: return instructionSQV();
  case 0x0a: return instructionSWV();
  case 0x0b: return instructionSTV();
  }
}

auto RSP::instructionSWV() -> void {
   u32 address = RS.u64 + int7(OP) * 16;
   u32 rotate = (OP >> 8 & 7) * 16;
  v128 vt{VPR[VTn].u128};
  v128 vs{vt.u128 << rotate | vt.u128 >> 128 - rotate};
  dmem.writeQuad(address, vs.u128);
}

auto RSP::instructionVABS() -> void {
  v128 vs0, slt;
  vs0  = _mm_cmpeq_epi16(VS, zero);
  slt  = _mm_srai_epi16(VS, 15);
  VD   = _mm_andnot_si128(vs0, VT);
  VD   = _mm_xor_si128(VD, slt);
  ACCL = _mm_sub_epi16(VD, slt);
  VD   = _mm_subs_epi16(VD, slt);
}

auto RSP::instructionVADD() -> void {
  v128 sum, min, max;
  sum  = _mm_add_epi16(VS, VT);
  ACCL = _mm_sub_epi16(sum, VCOL);
  min  = _mm_min_epi16(VS, VT);
  max  = _mm_max_epi16(VS, VT);
  min  = _mm_subs_epi16(min, VCOL);
  VD   = _mm_adds_epi16(min, max);
  VCOL = zero;
  VCOH = zero;
}

auto RSP::instructionVADDC() -> void {
  v128 sum;
  sum  = _mm_adds_epu16(VS, VT);
  ACCL = _mm_add_epi16(VS, VT);
  VCOL = _mm_cmpeq_epi16(sum, ACCL);
  VCOL = _mm_cmpeq_epi16(VCOL, zero);
  VCOH = zero;
  VD   = ACCL;
}

auto RSP::instructionVAND() -> void {
  ACCL = _mm_and_si128(VS, VT);
  VD   = ACCL;
}

auto RSP::instructionVCL() -> void {
  v128 nvt, diff, ncarry, nvce, diff0, lec1, lec2, leeq, geeq, le, ge, mask;
  nvt    = _mm_xor_si128(VT, VCOL);
  nvt    = _mm_sub_epi16(nvt, VCOL);
  diff   = _mm_sub_epi16(VS, nvt);
  ncarry = _mm_adds_epu16(VS, VT);
  ncarry = _mm_cmpeq_epi16(diff, ncarry);
  nvce   = _mm_cmpeq_epi16(VCE, zero);
  diff0  = _mm_cmpeq_epi16(diff, zero);
  lec1   = _mm_and_si128(diff0, ncarry);
  lec1   = _mm_and_si128(nvce, lec1);
  lec2   = _mm_or_si128(diff0, ncarry);
  lec2   = _mm_and_si128(VCE, lec2);
  leeq   = _mm_or_si128(lec1, lec2);
  geeq   = _mm_subs_epu16(VT, VS);
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
  v128 nvt, diff, diff0, vtn, dlez, dgez, mask;
  VCOL  = _mm_xor_si128(VS, VT);
  VCOL  = _mm_cmplt_epi16(VCOL, zero);
  nvt   = _mm_xor_si128(VT, VCOL);
  nvt   = _mm_sub_epi16(nvt, VCOL);
  diff  = _mm_sub_epi16(VS, nvt);
  diff0 = _mm_cmpeq_epi16(diff, zero);
  vtn   = _mm_cmplt_epi16(VT, zero);
  dlez  = _mm_cmplt_epi16(diff, zero);
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
  //todo
}

auto RSP::instructionVEQ() -> void {
  //todo
}

auto RSP::instructionVGE() -> void {
  //todo
}

auto RSP::instructionVLT() -> void {
  //todo
}

auto RSP::instructionVMACF() -> void {
  //todo
}

auto RSP::instructionVMACQ() -> void {
  //todo
}

auto RSP::instructionVMACU() -> void {
  //todo
}

auto RSP::instructionVMADH() -> void {
  //todo
}

auto RSP::instructionVMADL() -> void {
  //todo
}

auto RSP::instructionVMADM() -> void {
  //todo
}

auto RSP::instructionVMADN() -> void {
  //todo
}

auto RSP::instructionVMOV() -> void {
  u32 de = OP >> 11 & 31;
  u32 e  = OP >> 21 & 15;
  e = e < 2 ? de & 7 : e < 4 ? e & 1 | de & 6 : e < 8 ? e & 3 | de & 4 : e & 7;
  VD.element(de & 7) = VT.element(e);
  ACCL.u128 = VT.u128;
}

auto RSP::instructionVMRG() -> void {
  //todo
}

auto RSP::instructionVMUDH() -> void {
  //todo
}

auto RSP::instructionVMUDL() -> void {
  //todo
}

auto RSP::instructionVMUDM() -> void {
  //todo
}

auto RSP::instructionVMUDN() -> void {
  //todo
}

auto RSP::instructionVMULF() -> void {
  //todo
}

auto RSP::instructionVMULQ() -> void {
  //todo
}

auto RSP::instructionVMULU() -> void {
  //todo
}

auto RSP::instructionVNAND() -> void {
  ACCL = _mm_and_si128(VS, VT);
  ACCL = _mm_xor_si128(ACCL, invert);
  VD   = ACCL;
}

auto RSP::instructionVNE() -> void {
  //todo
}

auto RSP::instructionVNOP() -> void {
  //todo
}

auto RSP::instructionVNOR() -> void {
  ACCL = _mm_or_si128(VS, VT);
  ACCL = _mm_xor_si128(ACCL, invert);
  VD   = ACCL;
}

auto RSP::instructionVNXOR() -> void {
  ACCL = _mm_xor_si128(VS, VT);
  ACCL = _mm_xor_si128(ACCL, invert);
  VD   = ACCL;
}

auto RSP::instructionVOR() -> void {
  ACCL = _mm_or_si128(VS, VT);
  VD   = ACCL;
}

auto RSP::instructionVRCP() -> void {
  //todo
}

auto RSP::instructionVRCPH() -> void {
  //todo
}

auto RSP::instructionVRCPL() -> void {
  //todo
}

auto RSP::instructionVRNDN() -> void {
  //todo
}

auto RSP::instructionVRNDP() -> void {
  //todo
}

auto RSP::instructionVRSQ() -> void {
  //todo
}

auto RSP::instructionVRSQH() -> void {
  //todo
}

auto RSP::instructionVRSQL() -> void {
  //todo
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
  v128 udiff, sdiff, ov;
  udiff = _mm_sub_epi16(VT, VCOL);
  sdiff = _mm_subs_epi16(VT, VCOL);
  ACCL  = _mm_sub_epi16(VS, udiff);
  ov    = _mm_cmpgt_epi16(sdiff, udiff);
  VD    = _mm_subs_epi16(VS, sdiff);
  VD    = _mm_adds_epi16(VD, ov);
}

auto RSP::instructionVSUBC() -> void {
  v128 equal, udiff, diff0;
  udiff = _mm_subs_epu16(VS, VT);
  equal = _mm_cmpeq_epi16(VS, VT);
  diff0 = _mm_cmpeq_epi16(udiff, zero);
  VCOH  = _mm_cmpeq_epi16(equal, zero);
  VCOL  = _mm_andnot_si128(equal, diff0);
  ACCL  = _mm_sub_epi16(VS, VT);
  VD    = ACCL;
}

auto RSP::instructionVXOR() -> void {
  ACCL = _mm_xor_si128(VS, VT);
  VD   = ACCL;
}
