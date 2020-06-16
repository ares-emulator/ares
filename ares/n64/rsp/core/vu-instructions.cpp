auto RSP::instructionCFC2(r32& rt, u8 rd) -> void {
  r128 hi, lo;
  switch(rd & 3) {
  case 0x00: hi = VCOH; lo = VCOL; break;
  case 0x01: hi = VCCH; lo = VCCL; break;
  case 0x02: hi = zero; lo = VCE;  break;
  case 0x03: hi = zero; lo = VCE;  break;  //unverified
  }

  if constexpr(Accuracy::RSP::SISD) {
    rt.u32 = 0;
    for(uint n : range(8)) {
      rt.u32 |= lo.get(n) << 0 + n;
      rt.u32 |= hi.get(n) << 8 + n;
    }
    rt.u32 = i16(rt.u32);
  }

  if constexpr(Accuracy::RSP::SIMD) {
    static const v128 reverse = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);
    rt.u32 = i16(_mm_movemask_epi8(_mm_shuffle_epi8(_mm_packs_epi16(hi, lo), reverse)));
  }
}

auto RSP::instructionCTC2(cr32& rt, u8 rd) -> void {
  maybe<r128&> hi, lo;
  r128 null;
  switch(rd & 3) {
  case 0x00: hi = VCOH; lo = VCOL; break;
  case 0x01: hi = VCCH; lo = VCCL; break;
  case 0x02: hi = null; lo = VCE;  break;
  case 0x03: hi = null; lo = VCE;  break;  //unverified
  }

  if constexpr(Accuracy::RSP::SISD) {
    for(uint n : range(8)) {
      lo->set(n, rt.u32 & 1 << 0 + n);
      hi->set(n, rt.u32 & 1 << 8 + n);
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    static const v128 mask = _mm_set_epi16(0x0101, 0x0202, 0x0404, 0x0808, 0x1010, 0x2020, 0x4040, 0x8080);
    lo->v128 = _mm_cmpeq_epi8(_mm_and_si128(_mm_shuffle_epi8(r128{~rt.u32 >> 0}, zero), mask), zero);
    hi->v128 = _mm_cmpeq_epi8(_mm_and_si128(_mm_shuffle_epi8(r128{~rt.u32 >> 8}, zero), mask), zero);
  }
}

auto RSP::instructionLBV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm;
  vt.byte(e) = dmem.readByte(address);
}

auto RSP::instructionLDV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 8;
  auto start = e;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    vt.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLFV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = e >> 1;
  auto end = start + 4;
  for(uint offset = start; offset < end; offset++) {
    vt.element(offset & 7) = dmem.readByte(address) << 7;
    address += 4;
  }
}

auto RSP::instructionLHV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  for(uint offset = 0; offset < 8; offset++) {
    vt.element(offset) = dmem.readByte(address + (16 - e + offset * 2 & 15)) << 7;
  }
}

auto RSP::instructionLLV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 4;
  auto start = e;
  auto end = start + 4;
  for(uint offset = start; offset < end; offset++) {
    vt.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLPV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 8;
  for(uint offset = 0; offset < 8; offset++) {
    vt.element(offset) = dmem.readByte(address + (16 - e + offset & 15)) << 8;
  }
}

auto RSP::instructionLQV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = e;
  auto end = 16 - (address & 15);
  for(uint offset = start; offset < end; offset++) {
    vt.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLRV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto index = e;
  auto start = 16 - ((address & 15) - index);
  address &= ~15;
  for(uint offset = start; offset < 16; offset++) {
    vt.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLSV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 2;
  auto start = e;
  auto end = start + 2;
  for(uint offset = start; offset < end; offset++) {
    vt.byte(offset & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLTV(u8 vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = vt;
  auto end = min(32, start + 8);
  address = (address + 8 & ~15) + (e & 1);
  for(uint offset = start; offset < end; offset++) {
    auto byte = (8 - (e >> 1) + (offset - start)) << 1;
    VPR[offset].byte(byte + 0 & 15) = dmem.readByte(address++);
    VPR[offset].byte(byte + 1 & 15) = dmem.readByte(address++);
  }
}

auto RSP::instructionLUV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 8;
  for(uint offset = 0; offset < 8; offset++) {
    vt.element(offset) = dmem.readByte(address + (16 - e + offset & 15)) << 7;
  }
}

auto RSP::instructionLWV(r128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = 16 - e;
  auto end = e + 16;
  for(uint offset = start; offset < end; offset++) {
    vt.byte(offset & 15) = dmem.readByte(address);
    address += 4;
  }
}

auto RSP::instructionMFC2(r32& rt, cr128& vs, u8 e) -> void {
  auto hi = vs.byte(e + 0 & 15);
  auto lo = vs.byte(e + 1 & 15);
  rt.u32 = i16(hi << 8 | lo << 0);
}

auto RSP::instructionMTC2(cr32& rt, r128& vs, u8 e) -> void {
  vs.byte(e + 0 & 15) = rt.u32 >> 8;
  vs.byte(e + 1 & 15) = rt.u32 >> 0;
}

auto RSP::instructionSBV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm;
  dmem.writeByte(address, vt.byte(e));
}

auto RSP::instructionSDV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 8;
  auto start = e;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, vt.byte(offset & 15));
  }
}

auto RSP::instructionSFV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = e >> 1;
  auto end = start + 4;
  auto base = address & 15;
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address + (base & 15), vt.element(offset & 7) >> 7);
    base += 4;
  }
}

auto RSP::instructionSHV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  for(uint offset = 0; offset < 8; offset++) {
    auto byte = e + offset * 2;
    auto value = vt.byte(byte + 0 & 15) << 1 | vt.byte(byte + 1 & 15) >> 7;
    dmem.writeByte(address, value);
    address += 2;
  }
}

auto RSP::instructionSLV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 4;
  auto start = e;
  auto end = start + 4;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, vt.byte(offset & 15));
  }
}

auto RSP::instructionSPV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 8;
  auto start = e;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    if((offset & 15) < 8) {
      dmem.writeByte(address++, vt.byte((offset & 7) << 1));
    } else {
      dmem.writeByte(address++, vt.element(offset & 7) >> 7);
    }
  }
}

auto RSP::instructionSQV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = e;
  auto end = start + (16 - (address & 15));
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, vt.byte(offset & 15));
  }
}

auto RSP::instructionSRV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = e;
  auto end = start + (address & 15);
  auto base = 16 - (address & 15);
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, vt.byte(offset + base & 15));
  }
}

auto RSP::instructionSSV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 2;
  auto start = e;
  auto end = start + 2;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address++, vt.byte(offset & 15));
  }
}

auto RSP::instructionSTV(u8 vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = vt;
  auto end = min(32, start + 8);
  auto element = 8 - (e >> 1);
  auto base = (address & 15) + (element << 1);
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeHalfUnaligned(address + (base & 15), VPR[offset].element(element++ & 7));
    base += 2;
  }
}

auto RSP::instructionSUV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 8;
  auto start = e;
  auto end = start + 8;
  for(uint offset = start; offset < end; offset++) {
    if((offset & 15) < 8) {
      dmem.writeByte(address++, vt.element(offset & 7) >> 7);
    } else {
      dmem.writeByte(address++, vt.byte((offset & 7) << 1));
    }
  }
}

auto RSP::instructionSWV(cr128& vt, u8 e, cr32& rs, i8 imm) -> void {
  auto address = rs.u32 + imm * 16;
  auto start = e;
  auto end = start + 16;
  auto base = address & 15;
  address &= ~15;
  for(uint offset = start; offset < end; offset++) {
    dmem.writeByte(address + (base++ & 15), vt.byte(offset & 15));
  }
}

auto RSP::instructionVABS(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    r128 vte = vt(e);
    for(uint n : range(8)) {
      if(vs.i16(n) < 0) {
        if(vte.i16(n) == -32768) vte.i16(n) = -32767;
        ACCL.i16(n) = -vte.i16(n);
      } else if(vs.i16(n) > 0) {
        ACCL.i16(n) = +vte.i16(n);
      } else {
        ACCL.i16(n) = 0;
      }
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vs0, slt;
    vs0  = _mm_cmpeq_epi16(vs, zero);
    slt  = _mm_srai_epi16(vs, 15);
    vd   = _mm_andnot_si128(vs0, vt(e));
    vd   = _mm_xor_si128(vd, slt);
    ACCL = _mm_sub_epi16(vd, slt);
    vd   = _mm_subs_epi16(vd, slt);
  }
}

auto RSP::instructionVADD(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      i32 result = vs.i16(n) + vte.i16(n) + VCOL.get(n);
      ACCL.i16(n) = result;
      vd.i16(n) = sclamp<16>(result);
    }
    VCOL = zero;
    VCOH = zero;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), sum, min, max;
    sum  = _mm_add_epi16(vs, vte);
    ACCL = _mm_sub_epi16(sum, VCOL);
    min  = _mm_min_epi16(vs, vte);
    max  = _mm_max_epi16(vs, vte);
    min  = _mm_subs_epi16(min, VCOL);
    vd   = _mm_adds_epi16(min, max);
    VCOL = zero;
    VCOH = zero;
  }
}

auto RSP::instructionVADDC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      u32 result = vs.u16(n) + vte.u16(n);
      ACCL.u16(n) = result;
      VCOL.set(n, result >> 16);
    }
    VCOH = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), sum;
    sum  = _mm_adds_epu16(vs, vte);
    ACCL = _mm_add_epi16(vs, vte);
    VCOL = _mm_cmpeq_epi16(sum, ACCL);
    VCOL = _mm_cmpeq_epi16(VCOL, zero);
    VCOH = zero;
    vd   = ACCL;
  }
}

auto RSP::instructionVAND(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    r128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = vs.u16(n) & vte.u16(n);
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_and_si128(vs, vt(e));
    vd   = ACCL;
  }
}

auto RSP::instructionVCH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      if((vs.i16(n) ^ vte.i16(n)) < 0) {
        i16 result = vs.i16(n) + vte.i16(n);
        ACCL.i16(n) = (result <= 0 ? -vte.i16(n) : vs.i16(n));
        VCCL.set(n, result <= 0);
        VCCH.set(n, vte.i16(n) < 0);
        VCOL.set(n, 1);
        VCOH.set(n, result != 0 && vs.u16(n) != (vte.u16(n) ^ 0xffff));
        VCE.set(n, result == -1);
      } else {
        i16 result = vs.i16(n) - vte.i16(n);
        ACCL.i16(n) = (result >= 0 ? vte.i16(n) : vs.i16(n));
        VCCL.set(n, vte.i16(n) < 0);
        VCCH.set(n, result >= 0);
        VCOL.set(n, 0);
        VCOH.set(n, result != 0 && vs.u16(n) != (vte.u16(n) ^ 0xffff));
        VCE.set(n, 0);
      }
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), nvt, diff, diff0, vtn, dlez, dgez, mask;
    VCOL  = _mm_xor_si128(vs, vte);
    VCOL  = _mm_cmplt_epi16(VCOL, zero);
    nvt   = _mm_xor_si128(vte, VCOL);
    nvt   = _mm_sub_epi16(nvt, VCOL);
    diff  = _mm_sub_epi16(vs, nvt);
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
    ACCL  = _mm_blendv_epi8(vs, nvt, mask);
    vd    = ACCL;
  }
}

auto RSP::instructionVCL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      if(VCOL.get(n)) {
        if(VCOH.get(n)) {
          ACCL.u16(n) = VCCL.get(n) ? -vte.u16(n) : vs.u16(n);
        } else if(VCE.get(n)) {
          ACCL.u16(n) = VCCL.set(n, vs.u16(n) + vte.u16(n) <= 0xffff) ? -vte.u16(n) : vs.u16(n);
        } else {
          ACCL.u16(n) = VCCL.set(n, vs.u16(n) + vte.u16(n) == 0) ? -vte.u16(n) : vs.u16(n);
        }
      } else {
        if(VCOH.get(n)) {
          ACCL.u16(n) = VCCH.get(n) ? vte.u16(n) : vs.u16(n);
        } else {
          ACCL.u16(n) = VCCH.set(n, (i32)vs.u16(n) - (i32)vte.u16(n) >= 0) ? vte.u16(n) : vs.u16(n);
        }
      }
    }
    VCOL = zero;
    VCOH = zero;
    VCE = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), nvt, diff, ncarry, nvce, diff0, lec1, lec2, leeq, geeq, le, ge, mask;
    nvt    = _mm_xor_si128(vte, VCOL);
    nvt    = _mm_sub_epi16(nvt, VCOL);
    diff   = _mm_sub_epi16(vs, nvt);
    ncarry = _mm_adds_epu16(vs, vte);
    ncarry = _mm_cmpeq_epi16(diff, ncarry);
    nvce   = _mm_cmpeq_epi16(VCE, zero);
    diff0  = _mm_cmpeq_epi16(diff, zero);
    lec1   = _mm_and_si128(diff0, ncarry);
    lec1   = _mm_and_si128(nvce, lec1);
    lec2   = _mm_or_si128(diff0, ncarry);
    lec2   = _mm_and_si128(VCE, lec2);
    leeq   = _mm_or_si128(lec1, lec2);
    geeq   = _mm_subs_epu16(vte, vs);
    geeq   = _mm_cmpeq_epi16(geeq, zero);
    le     = _mm_andnot_si128(VCOH, VCOL);
    le     = _mm_blendv_epi8(VCCL, leeq, le);
    ge     = _mm_or_si128(VCOL, VCOH);
    ge     = _mm_blendv_epi8(geeq, VCCH, ge);
    mask   = _mm_blendv_epi8(ge, le, VCOL);
    ACCL   = _mm_blendv_epi8(vs, nvt, mask);
    VCCH   = ge;
    VCCL   = le;
    VCOH   = zero;
    VCOL   = zero;
    VCE    = zero;
    vd     = ACCL;
  }
}

auto RSP::instructionVCR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      if((vs.i16(n) ^ vte.i16(n)) < 0) {
        VCCH.set(n, vte.i16(n) < 0);
        ACCL.u16(n) = VCCL.set(n, vs.i16(n) + vte.i16(n) + 1 <= 0) ? ~vte.u16(n) : vs.u16(n);
      } else {
        VCCL.set(n, vte.i16(n) < 0);
        ACCL.u16(n) = VCCH.set(n, vs.i16(n) - vte.i16(n) >= 0) ? vte.u16(n) : vs.u16(n);
      }
    }
    VCOL = zero;
    VCOH = zero;
    VCE = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), sign, dlez, dgez, nvt, mask;
    sign = _mm_xor_si128(vs, vte);
    sign = _mm_srai_epi16(sign, 15);
    dlez = _mm_and_si128(vs, sign);
    dlez = _mm_add_epi16(dlez, vte);
    VCCL = _mm_srai_epi16(dlez, 15);
    dgez = _mm_or_si128(vs, sign);
    dgez = _mm_min_epi16(dgez, vte);
    VCCH = _mm_cmpeq_epi16(dgez, vte);
    nvt  = _mm_xor_si128(vte, sign);
    mask = _mm_blendv_epi8(VCCH, VCCL, sign);
    ACCL = _mm_blendv_epi8(vs, nvt, mask);
    vd   = ACCL;
    VCOL = zero;
    VCOH = zero;
    VCE  = zero;
  }
}

auto RSP::instructionVEQ(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = VCCL.set(n, !VCE.get(n) && vs.u16(n) == vte.u16(n)) ? vs.u16(n) : vte.u16(n);
    }
    VCCH = zero;  //unverified
    VCOL = zero;
    VCOH = zero;
    VCE = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), eq;
    eq   = _mm_cmpeq_epi16(vs, vte);
    VCCL = _mm_andnot_si128(VCOH, eq);
    ACCL = _mm_blendv_epi8(vte, vs, VCCL);
    VCCH = zero;  //unverified
    VCOH = zero;
    VCOL = zero;
    vd   = ACCL;
  }
}

auto RSP::instructionVGE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = VCCL.set(n, vs.i16(n) > vte.i16(n) || (vs.i16(n) == vte.i16(n) && (!VCOL.get(n) || VCE.get(n)))) ? vs.u16(n) : vte.u16(n);
    }
    VCCH = zero;  //unverified
    VCOL = zero;
    VCOH = zero;
    VCE = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), eq, gt, es;
    eq   = _mm_cmpeq_epi16(vs, vte);
    gt   = _mm_cmpgt_epi16(vs, vte);
    es   = _mm_and_si128(VCOH, VCOL);
    eq   = _mm_andnot_si128(es, eq);
    VCCL = _mm_or_si128(gt, eq);
    ACCL = _mm_blendv_epi8(vte, vs, VCCL);
    VCCH = zero;
    VCOH = zero;
    VCOL = zero;
    vd   = ACCL;
  }
}

auto RSP::instructionVLT(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = VCCL.set(n, vs.i16(n) < vte.i16(n) || (vs.i16(n) == vte.i16(n) && VCOL.get(n) && !VCE.get(n))) ? vs.u16(n) : vte.u16(n);
    }
    VCCH = zero;  //unverified
    VCOL = zero;
    VCOH = zero;
    VCE = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), eq, lt;
    eq   = _mm_cmpeq_epi16(vs, vte);
    lt   = _mm_cmplt_epi16(vs, vte);
    eq   = _mm_and_si128(VCOH, eq);
    eq   = _mm_and_si128(VCOL, eq);
    VCCL = _mm_or_si128(lt, eq);
    ACCL = _mm_blendv_epi8(vte, vs, VCCL);
    VCCH = zero;
    VCOH = zero;
    VCOL = zero;
    vd   = ACCL;
  }
}

template<bool U>
auto RSP::instructionVMACF(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, accumulatorGet(n) + vs.i16(n) * vte.i16(n) * 2);
      if constexpr(U == 0) {
        vd.u16(n) = accumulatorSaturate(n, 1, 0x8000, 0x7fff);
      }
      if constexpr(U == 1) {
        vd.u16(n) = ACCH.i16(n) < 0 ? 0x0000 : (ACCH.i16(n) ^ ACCM.i16(n)) < 0 ? 0xffff : ACCM.u16(n);
      }
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), lo, md, hi, carry, omask;
    lo    = _mm_mullo_epi16(vs, vte);
    hi    = _mm_mulhi_epi16(vs, vte);
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
    if constexpr(!U) {
      lo = _mm_unpacklo_epi16(ACCM, ACCH);
      hi = _mm_unpackhi_epi16(ACCM, ACCH);
      vd = _mm_packs_epi32(lo, hi);
    } else {
      r128 mmask, hmask;
      mmask = _mm_srai_epi16(ACCM, 15);
      hmask = _mm_srai_epi16(ACCH, 15);
      md    = _mm_or_si128(mmask, ACCM);
      omask = _mm_cmpgt_epi16(ACCH, zero);
      md    = _mm_andnot_si128(hmask, md);
      vd    = _mm_or_si128(omask, md);
    }
  }
}

auto RSP::instructionVMACQ(r128& vd) -> void {
  for(uint n : range(8)) {
    i32 product = ACCH.element(n) << 16 | ACCM.element(n) << 0;
    if(product < 0 && !(product & 1 << 5)) product += 32;
    else if(product >= 0 && !(product & 1 << 5)) product -= 32;
    ACCH.element(n) = product >> 16;
    ACCM.element(n) = product >>  0;
    ACCL.element(n) = 0;  //unconfirmed
    vd.element(n) = sclamp<16>(product >> 1) & ~15;
  }
}

auto RSP::instructionVMADH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      i32 result = (accumulatorGet(n) >> 16) + vs.i16(n) * vte.i16(n);
      ACCH.u16(n) = result >> 16;
      ACCM.u16(n) = result >>  0;
      vd.u16(n) = accumulatorSaturate(n, 1, 0x8000, 0x7fff);
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), lo, hi, omask;
    lo    = _mm_mullo_epi16(vs, vte);
    hi    = _mm_mulhi_epi16(vs, vte);
    omask = _mm_adds_epu16(ACCM, lo);
    ACCM  = _mm_add_epi16(ACCM, lo);
    omask = _mm_cmpeq_epi16(ACCM, omask);
    omask = _mm_cmpeq_epi16(omask, zero);
    hi    = _mm_sub_epi16(hi, omask);
    ACCH  = _mm_add_epi16(ACCH, hi);
    lo    = _mm_unpacklo_epi16(ACCM, ACCH);
    hi    = _mm_unpackhi_epi16(ACCM, ACCH);
    vd    = _mm_packs_epi32(lo, hi);
  }
}

auto RSP::instructionVMADL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      u32 r1 = vs.u16(n) * vte.u16(n);
      u32 r2 = ACCL.u16(n) + (r1 >> 16);
      u32 r3 = ACCM.u16(n) + (r2 >> 16);
      ACCL.u16(n)  = r2;
      ACCM.u16(n)  = r3;
      ACCH.u16(n) += i16(r3 >> 16);
      vd.u16(n) = accumulatorSaturate(n, 0, 0x0000, 0xffff);
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), hi, omask, nhi, nmd, shi, smd, cmask, cval;
    hi    = _mm_mulhi_epu16(vs, vte);
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
    vd    = _mm_blendv_epi8(cval, ACCL, cmask);
  }
}

auto RSP::instructionVMADM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      u32 r1 = vs.i16(n) * vte.u16(n);
      u32 r2 = ACCL.u16(n) + u16(r1);
      u32 r3 = ACCM.u16(n) + (r1 >> 16) + (r2 >> 16);
      ACCL.u16(n)  = r2;
      ACCM.u16(n)  = r3;
      ACCH.u16(n) += (r3 >> 16) - (i32(r1) < 0);
      vd.u16(n) = accumulatorSaturate(n, 1, 0x8000, 0x7fff);
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), lo, hi, sign, vta, omask;
    lo    = _mm_mullo_epi16(vs, vte);
    hi    = _mm_mulhi_epu16(vs, vte);
    sign  = _mm_srai_epi16(vs, 15);
    vta   = _mm_and_si128(vte, sign);
    hi    = _mm_sub_epi16(hi, vta);
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
    vd    = _mm_packs_epi32(lo, hi);
  }
}

auto RSP::instructionVMADN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, accumulatorGet(n) + i64(vs.u16(n) * vte.i16(n)));
      vd.u16(n) = accumulatorSaturate(n, 0, 0x0000, 0xffff);
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), lo, hi, sign, vsa, omask, nhi, nmd, shi, smd, cmask, cval;
    lo    = _mm_mullo_epi16(vs, vte);
    hi    = _mm_mulhi_epu16(vs, vte);
    sign  = _mm_srai_epi16(vte, 15);
    vsa   = _mm_and_si128(vs, sign);
    hi    = _mm_sub_epi16(hi, vsa);
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
    vd    = _mm_blendv_epi8(cval, ACCL, cmask);
  }
}

auto RSP::instructionVMOV(r128& vd, u8 de, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    ACCL = vte;
    vd.u16(de) = vte.u16(de);
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = vt(e);
    vd.element(de) = ACCL.element(de);
  }
}

auto RSP::instructionVMRG(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = VCCL.get(n) ? vs.u16(n) : vte.u16(n);
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_blendv_epi8(vt(e), vs, VCCL);
    VCOH = zero;
    VCOL = zero;
    vd   = ACCL;
  }
}

auto RSP::instructionVMUDH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, i64(vs.i16(n) * vte.i16(n)) << 16);
      vd.u16(n) = accumulatorSaturate(n, 1, 0x8000, 0x7fff);
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), lo, hi;
    ACCL = zero;
    ACCM = _mm_mullo_epi16(vs, vte);
    ACCH = _mm_mulhi_epi16(vs, vte);
    lo   = _mm_unpacklo_epi16(ACCM, ACCH);
    hi   = _mm_unpackhi_epi16(ACCM, ACCH);
    vd   = _mm_packs_epi32(lo, hi);
  }
}

auto RSP::instructionVMUDL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, u16(vs.u16(n) * vte.u16(n) >> 16));
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_mulhi_epu16(vs, vt(e));
    ACCM = zero;
    ACCH = zero;
    vd   = ACCL;
  }
}

auto RSP::instructionVMUDM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, i32(vs.i16(n) * vte.u16(n)));
    }
    vd = ACCM;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), sign, vta;
    ACCL = _mm_mullo_epi16(vs, vte);
    ACCM = _mm_mulhi_epu16(vs, vte);
    sign = _mm_srai_epi16(vs, 15);
    vta  = _mm_and_si128(vte, sign);
    ACCM = _mm_sub_epi16(ACCM, vta);
    ACCH = _mm_srai_epi16(ACCM, 15);
    vd   = ACCM;
  }
}

auto RSP::instructionVMUDN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, i32(vs.u16(n) * vte.i16(n)));
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), sign, vsa;
    ACCL = _mm_mullo_epi16(vs, vte);
    ACCM = _mm_mulhi_epu16(vs, vte);
    sign = _mm_srai_epi16(vte, 15);
    vsa  = _mm_and_si128(vs, sign);
    ACCM = _mm_sub_epi16(ACCM, vsa);
    ACCH = _mm_srai_epi16(ACCM, 15);
    vd   = ACCL;
  }
}

template<bool U>
auto RSP::instructionVMULF(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      accumulatorSet(n, vs.i16(n) * vte.i16(n) * 2 + 0x8000);
      if constexpr(U == 0) {
        vd.u16(n) = ACCM.u16(n);
      }
      if constexpr(U == 1) {
        vd.u16(n) = ACCH.i16(n) < 0 ? 0x0000 : (ACCH.i16(n) ^ ACCM.i16(n)) < 0 ? 0xffff : ACCM.u16(n);
      }
    }
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), lo, hi, round, sign1, sign2, neq, eq, neg;
    lo    = _mm_mullo_epi16(vs, vte);
    round = _mm_cmpeq_epi16(zero, zero);
    sign1 = _mm_srli_epi16(lo, 15);
    lo    = _mm_add_epi16(lo, lo);
    round = _mm_slli_epi16(round, 15);
    hi    = _mm_mulhi_epi16(vs, vte);
    sign2 = _mm_srli_epi16(lo, 15);
    ACCL  = _mm_add_epi16(round, lo);
    sign1 = _mm_add_epi16(sign1, sign2);
    hi    = _mm_slli_epi16(hi, 1);
    neq   = _mm_cmpeq_epi16(vs, vte);
    ACCM  = _mm_add_epi16(hi, sign1);
    neg   = _mm_srai_epi16(ACCM, 15);
    if constexpr(!U) {
      eq   = _mm_and_si128(neq, neg);
      ACCH = _mm_andnot_si128(neq, neg);
      vd   = _mm_add_epi16(ACCM, eq);
    } else {
      ACCH = _mm_andnot_si128(neq, neg);
      hi   = _mm_or_si128(ACCM, neg);
      vd   = _mm_andnot_si128(ACCH, hi);
    }
  }
}

auto RSP::instructionVMULQ(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  cr128 vte = vt(e);
  for(uint n : range(8)) {
    i32 product = (i16)vs.element(n) * (i16)vte.element(n);
    if(product < 0) product += 31;  //round
    ACCH.element(n) = product >> 16;
    ACCM.element(n) = product >>  0;
    ACCL.element(n) = 0;  //unconfirmed
    vd.element(n) = sclamp<16>(product >> 1) & ~15;
  }
}

auto RSP::instructionVNAND(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = ~(vs.u16(n) & vte.u16(n));
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_and_si128(vs, vt(e));
    ACCL = _mm_xor_si128(ACCL, invert);
    vd   = ACCL;
  }
}

auto RSP::instructionVNE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = VCCL.set(n, vs.u16(n) != vte.u16(n) || VCE.get(n)) ? vs.u16(n) : vte.u16(n);
    }
    VCCH = zero;  //unverified
    VCOL = zero;
    VCOH = zero;
    VCE = zero;
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), eq, ne;
    eq   = _mm_cmpeq_epi16(vs, vte);
    ne   = _mm_cmpeq_epi16(eq, zero);
    VCCL = _mm_and_si128(VCOH, eq);
    VCCL = _mm_or_si128(VCCL, ne);
    ACCL = _mm_blendv_epi8(vte, vs, VCCL);
    VCCH = zero;
    VCOH = zero;
    VCOL = zero;
    vd   = ACCL;
  }
}

auto RSP::instructionVNOP() -> void {
}

auto RSP::instructionVNOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = ~(vs.u16(n) | vte.u16(n));
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_or_si128(vs, vt(e));
    ACCL = _mm_xor_si128(ACCL, invert);
    vd   = ACCL;
  }
}

auto RSP::instructionVNXOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = ~(vs.u16(n) ^ vte.u16(n));
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_xor_si128(vs, vt(e));
    ACCL = _mm_xor_si128(ACCL, invert);
    vd   = ACCL;
  }
}

auto RSP::instructionVOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = vs.u16(n) | vte.u16(n);
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_or_si128(vs, vt(e));
    vd   = ACCL;
  }
}

template<bool L>
auto RSP::instructionVRCP(r128& vd, u8 de, cr128& vt, u8 e) -> void {
  i32 result = 0;
  i32 input = L && DIVDP ? DIVIN << 16 | vt.element(e & 7) : i16(vt.element(e & 7));
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
  ACCL = vt(e);
  vd.element(de) = result;
}

auto RSP::instructionVRCPH(r128& vd, u8 de, cr128& vt, u8 e) -> void {
  ACCL  = vt(e);
  DIVDP = 1;
  DIVIN = vt.element(e & 7);
  vd.element(de) = DIVOUT;
}

template<bool D>
auto RSP::instructionVRND(r128& vd, u8 vs, cr128& vt, u8 e) -> void {
  cr128 vte = vt(e);
  for(uint n : range(8)) {
    i32 product = (i16)vte.element(n);
    if(vs & 1) product <<= 16;
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
    vd.element(n) = acc >> 16;
  }
}

template<bool L>
auto RSP::instructionVRSQ(r128& vd, u8 de, cr128& vt, u8 e) -> void {
  i32 result = 0;
  i32 input = L && DIVDP ? DIVIN << 16 | vt.element(e & 7) : i16(vt.element(e & 7));
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
  ACCL = vt(e);
  vd.element(de) = result;
}

auto RSP::instructionVRSQH(r128& vd, u8 de, cr128& vt, u8 e) -> void {
  ACCL  = vt(e);
  DIVDP = 1;
  DIVIN = vt.element(e & 7);
  vd.element(de) = DIVOUT;
}

auto RSP::instructionVSAR(r128& vd, cr128& vs, u8 e) -> void {
  switch(e) {
  case 0x8: vd = ACCH; break;
  case 0x9: vd = ACCM; break;
  case 0xa: vd = ACCL; break;
  default:  vd = zero; break;
  }
}

auto RSP::instructionVSUB(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      i32 result = vs.i16(n) - vte.i16(n) - VCOL.get(n);
      ACCL.i16(n) = result;
      vd.i16(n) = sclamp<16>(result);
    }
    VCOL = zero;
    VCOH = zero;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), udiff, sdiff, ov;
    udiff = _mm_sub_epi16(vte, VCOL);
    sdiff = _mm_subs_epi16(vte, VCOL);
    ACCL  = _mm_sub_epi16(vs, udiff);
    ov    = _mm_cmpgt_epi16(sdiff, udiff);
    vd    = _mm_subs_epi16(vs, sdiff);
    vd    = _mm_adds_epi16(vd, ov);
    VCOL  = zero;
    VCOH  = zero;
  }
}

auto RSP::instructionVSUBC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      u32 result = vs.u16(n) - vte.u16(n);
      ACCL.u16(n) = result;
      VCOL.set(n, result >> 16);
      VCOH.set(n, result != 0);
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    r128 vte = vt(e), equal, udiff, diff0;
    udiff = _mm_subs_epu16(vs, vte);
    equal = _mm_cmpeq_epi16(vs, vte);
    diff0 = _mm_cmpeq_epi16(udiff, zero);
    VCOH  = _mm_cmpeq_epi16(equal, zero);
    VCOL  = _mm_andnot_si128(equal, diff0);
    ACCL  = _mm_sub_epi16(vs, vte);
    vd    = ACCL;
  }
}

auto RSP::instructionVXOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void {
  if constexpr(Accuracy::RSP::SISD) {
    cr128 vte = vt(e);
    for(uint n : range(8)) {
      ACCL.u16(n) = vs.u16(n) ^ vte.u16(n);
    }
    vd = ACCL;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    ACCL = _mm_xor_si128(vs, vt(e));
    vd   = ACCL;
  }
}
