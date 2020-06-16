auto RSP::r128::operator()(uint index) const -> r128 {
  if constexpr(Accuracy::RSP::SISD) {
    r128 v{*this};
    switch(index) {
    case  0: break;
    case  1: break;
    case  2: v.u16(1) = v.u16(0); v.u16(3) = v.u16(2); v.u16(5) = v.u16(4); v.u16(7) = v.u16(6); break;
    case  3: v.u16(0) = v.u16(1); v.u16(2) = v.u16(3); v.u16(4) = v.u16(5); v.u16(6) = v.u16(7); break;
    case  4: v.u16(1) = v.u16(2) = v.u16(3) = v.u16(0); v.u16(5) = v.u16(6) = v.u16(7) = v.u16(4); break;
    case  5: v.u16(0) = v.u16(2) = v.u16(3) = v.u16(1); v.u16(4) = v.u16(6) = v.u16(7) = v.u16(5); break;
    case  6: v.u16(0) = v.u16(1) = v.u16(3) = v.u16(2); v.u16(4) = v.u16(5) = v.u16(7) = v.u16(6); break;
    case  7: v.u16(0) = v.u16(1) = v.u16(2) = v.u16(3); v.u16(4) = v.u16(5) = v.u16(6) = v.u16(7); break;
    case  8: for(uint n : range(8)) v.u16(n) = v.u16(0); break;
    case  9: for(uint n : range(8)) v.u16(n) = v.u16(1); break;
    case 10: for(uint n : range(8)) v.u16(n) = v.u16(2); break;
    case 11: for(uint n : range(8)) v.u16(n) = v.u16(3); break;
    case 12: for(uint n : range(8)) v.u16(n) = v.u16(4); break;
    case 13: for(uint n : range(8)) v.u16(n) = v.u16(5); break;
    case 14: for(uint n : range(8)) v.u16(n) = v.u16(6); break;
    case 15: for(uint n : range(8)) v.u16(n) = v.u16(7); break;
    }
    return v;
  }

  if constexpr(Accuracy::RSP::SIMD) {
    static const __m128i shuffle[16] = {
      //vector
      _mm_set_epi8(15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),  //01234567
      _mm_set_epi8(15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0),  //01234567
      //scalar quarter
      _mm_set_epi8(15,14,15,14,11,10,11,10, 7, 6, 7, 6, 3, 2, 3, 2),  //00224466
      _mm_set_epi8(13,12,13,12, 9, 8, 9, 8, 5, 4, 5, 4, 1, 0, 1, 0),  //11335577
      //scalar half
      _mm_set_epi8(15,14,15,14,15,14,15,14, 7, 6, 7, 6, 7, 6, 7, 6),  //00004444
      _mm_set_epi8(13,12,13,12,13,12,13,12, 5, 4, 5, 4, 5, 4, 5, 4),  //11115555
      _mm_set_epi8(11,10,11,10,11,10,11,10, 3, 2, 3, 2, 3, 2, 3, 2),  //22226666
      _mm_set_epi8( 9, 8, 9, 8, 9, 8, 9, 8, 1, 0, 1, 0, 1, 0, 1, 0),  //33337777
      //scalar whole
      _mm_set_epi8(15,14,15,14,15,14,15,14,15,14,15,14,15,14,15,14),  //00000000
      _mm_set_epi8(13,12,13,12,13,12,13,12,13,12,13,12,13,12,13,12),  //11111111
      _mm_set_epi8(11,10,11,10,11,10,11,10,11,10,11,10,11,10,11,10),  //22222222
      _mm_set_epi8( 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8, 9, 8),  //33333333
      _mm_set_epi8( 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6, 7, 6),  //44444444
      _mm_set_epi8( 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4, 5, 4),  //55555555
      _mm_set_epi8( 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2, 3, 2),  //66666666
      _mm_set_epi8( 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0),  //77777777
    };
    //todo: benchmark to see if testing for cases 0&1 to return value directly is faster
    return {uint128_t(_mm_shuffle_epi8(v128, shuffle[index]))};
  }
}

auto RSP::accumulatorGet(uint index) const -> u64 {
  return (u64)ACCH.u16(index) << 32 | (u64)ACCM.u16(index) << 16 | (u64)ACCL.u16(index) << 0;
}

auto RSP::accumulatorSet(uint index, u64 value) -> void {
  ACCH.u16(index) = value >> 32;
  ACCM.u16(index) = value >> 16;
  ACCL.u16(index) = value >>  0;
}

auto RSP::accumulatorSaturate(uint index, bool slice, u16 negative, u16 positive) const -> u16 {
  if(ACCH.i16(index) < 0) {
    if(ACCH.u16(index) != 0xffff) return negative;
    if(ACCM.i16(index) >= 0) return negative;
  } else {
    if(ACCH.u16(index) != 0x0000) return positive;
    if(ACCM.i16(index) < 0) return positive;
  }
  return !slice ? ACCL.u16(index) : ACCM.u16(index);
}
