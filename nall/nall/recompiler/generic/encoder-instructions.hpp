#pragma once

//{
  //0 operand instructions

  auto brk() {
    sljit_emit_op0(compiler, SLJIT_BREAKPOINT);
  }

  //1 operand instructions

#define OP1(name, op) \
  template<typename T, typename U> \
  auto name(T x, U y) { \
    sljit_emit_op1(compiler, \
                   SLJIT_##op, \
                   x.fst, x.snd, \
                   y.fst, y.snd); \
  }

  OP1(mov32, MOV32)
  OP1(mov64, MOV)
  OP1(mov32_u8, MOV32_U8)
  OP1(mov64_u8, MOV_U8)
  OP1(mov32_s8, MOV32_S8)
  OP1(mov64_s8, MOV_S8)
  OP1(mov32_u16, MOV32_U16)
  OP1(mov64_u16, MOV_U16)
  OP1(mov32_s16, MOV32_S16)
  OP1(mov64_s16, MOV_S16)
  OP1(mov64_u32, MOV_U32)
  OP1(mov64_s32, MOV_S32)
  OP1(rev32, REV_U32)
  OP1(rev32_u16, REV32_U16)
  OP1(rev32_s16, REV32_S16)
#undef OP1

  auto mov32(freg x, reg y) -> void {
    sljit_emit_fcopy(compiler, SLJIT_COPY32_TO_F32, x.fst, y.fst);
  }

  auto mov32(reg x, freg y) -> void {
    sljit_emit_fcopy(compiler, SLJIT_COPY32_FROM_F32, y.fst, x.fst);
  }

  template<typename T, typename U, typename V, typename W>
  auto lmul64_uw(T lo, U hi, V x, W y) {
    mov64(reg(0), x);
    mov64(reg(1), y);
    sljit_emit_op0(compiler, SLJIT_LMUL_UW);
    mov64(lo, reg(0));
    mov64(hi, reg(1));
  }

  template<typename T, typename U, typename V, typename W>
  auto lmul64_sw(T lo, U hi, V x, W y) {
    mov64(reg(0), x);
    mov64(reg(1), y);
    sljit_emit_op0(compiler, SLJIT_LMUL_SW);
    mov64(lo, reg(0));
    mov64(hi, reg(1));
  }

  template<typename T, typename U, typename V, typename W>
  auto divmod64_sw(T quotient, U remainder, V x, W y) {
    mov64(reg(0), x);
    mov64(reg(1), y);
    sljit_emit_op0(compiler, SLJIT_DIVMOD_SW);
    mov64(quotient, reg(0));
    mov64(remainder, reg(1));
  }

  template<typename T, typename U, typename V, typename W>
  auto divmod64_uw(T quotient, U remainder, V x, W y) {
    mov64(reg(0), x);
    mov64(reg(1), y);
    sljit_emit_op0(compiler, SLJIT_DIVMOD_UW);
    mov64(quotient, reg(0));
    mov64(remainder, reg(1));
  }

  template<typename T, typename U, typename V, typename W>
  auto divmod32_sw(T quotient, U remainder, V x, W y) {
    mov64_s32(reg(0), x);
    mov64_s32(reg(1), y);
    sljit_emit_op0(compiler, SLJIT_DIVMOD_S32);
    mov64_s32(quotient, reg(0));
    mov64_s32(remainder, reg(1));
  }

  template<typename T, typename U, typename V, typename W>
  auto divmod32_uw(T quotient, U remainder, V x, W y) {
    mov64_u32(reg(0), x);
    mov64_u32(reg(1), y);
    sljit_emit_op0(compiler, SLJIT_DIVMOD_U32);
    mov64_s32(quotient, reg(0));
    mov64_s32(remainder, reg(1));
  }

  //2 operand instructions

#define OP2(name, op) \
  template<typename T, typename U, typename V> \
  auto name(T x, U y, V z, sljit_s32 flags = 0) { \
    sljit_emit_op2(compiler, \
                   SLJIT_##op | flags, \
                   x.fst, x.snd, \
                   y.fst, y.snd, \
                   z.fst, z.snd); \
  } \
  template<typename U, typename V> \
  auto name(unused, U y, V z, sljit_s32 flags = 0) { \
    sljit_emit_op2u(compiler, \
                   SLJIT_##op | flags, \
                   y.fst, y.snd, \
                   z.fst, z.snd); \
  }

  OP2(add32, ADD32)
  OP2(add64, ADD)
  OP2(addc32, ADDC32)
  OP2(addc64, ADDC)
  OP2(sub32, SUB32)
  OP2(sub64, SUB)
  OP2(subc32, SUBC32)
  OP2(subc64, SUBC)
  OP2(mul32, MUL32)
  OP2(mul64, MUL)
  OP2(and32, AND32)
  OP2(and64, AND)
  OP2(or32, OR32)
  OP2(or64, OR)
  OP2(xor32, XOR32)
  OP2(xor64, XOR)
  OP2(shl32, SHL32)
  OP2(shl64, SHL)
  OP2(mshl32, MSHL32)
  OP2(mshl64, MSHL)
  OP2(lshr32, LSHR32)
  OP2(lshr64, LSHR)
  OP2(mlshr32, MLSHR32)
  OP2(mlshr64, MLSHR)
  OP2(ashr32, ASHR32)
  OP2(ashr64, ASHR)
  OP2(mashr32, MASHR32)
  OP2(mashr64, MASHR)
  OP2(rotl32, ROTL32)
  OP2(rotl64, ROTL)
  OP2(rotr32, ROTR32)
  OP2(rotr64, ROTR)
#undef OP2

  //compare instructions

#define OPC(name, op) \
  template<typename T, typename U> \
  auto name(T x, U y, sljit_s32 flags) { \
    sljit_emit_op2u(compiler, \
                    SLJIT_##op | flags, \
                    x.fst, x.snd, \
                    y.fst, y.snd); \
  }

  OPC(cmp32, SUB32)
  OPC(cmp64, SUB)
  OPC(test32, AND32)
  OPC(test64, AND)
#undef OPC

  template<typename T, typename U>
  auto cmp32_jump(T x, U y, sljit_s32 flags) -> sljit_jump* {
    return sljit_emit_cmp(compiler,
                          SLJIT_32 | flags,
                          x.fst, x.snd,
                          y.fst, y.snd);
  }

  //flag instructions

#define OPF(name, op) \
  template<typename T> \
  auto name(T x, sljit_s32 flags) { \
    sljit_emit_op_flags(compiler, \
                        SLJIT_##op, \
                        x.fst, x.snd, \
                        flags); \
  }

  OPF(mov32_f, MOV32)
  OPF(mov64_f, MOV)
  OPF(and32_f, AND32)
  OPF(and64_f, AND)
  OPF(or32_f, OR32)
  OPF(or64_f, OR)
  OPF(xor32_f, XOR32)
  OPF(xor64_f, XOR)
#undef OPF

  //meta instructions


  auto lea(reg r, sreg base, sljit_sw offset) {
    add64(r, base, imm(offset));
  }

  auto mov128(mem dst, mem src) -> void {
    static constexpr sljit_s32 kSimdTmp = SLJIT_FR(6);
    static constexpr sljit_s32 kSimdType = SLJIT_SIMD_REG_128 | SLJIT_SIMD_ELEM_8 | SLJIT_SIMD_MEM_UNALIGNED;
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_LOAD | kSimdType, kSimdTmp, src.fst, src.snd);
    sljit_emit_simd_mov(compiler, SLJIT_SIMD_STORE | kSimdType, kSimdTmp, dst.fst, dst.snd);
  }

  template<typename T, typename U, typename V>
  auto or8(T x, U y, V z, reg scratch) -> void {
#if defined(ARCHITECTURE_AMD64)
    if(SLJIT_IS_MEM(x.fst) && SLJIT_IS_MEM1(x.fst) && x.fst == y.fst && x.snd == y.snd && SLJIT_IS_REG(z.fst)) {
      s32 base = sljit_get_register_index(SLJIT_GP_REGISTER, SLJIT_EXTRACT_REG(x.fst));
      s32 src = sljit_get_register_index(SLJIT_GP_REGISTER, z.fst);
      if(base >= 0 && src >= 0 && (sljit_sw)(s32)x.snd == x.snd) {
        u8 opcode[10];
        u32 n = 0;
        u8 rex = 0x40u | (u8(src) >> 3 & 1) << 2 | (u8(base) >> 3 & 1);
        if(rex != 0x40u || (u8(src) & 7) >= 4) opcode[n++] = rex;
        opcode[n++] = 0x08;
        opcode[n++] = 0x80u | (u8(src) & 7) << 3 | (u8(base) & 7);
        if((u8(base) & 7) == 4) opcode[n++] = 0x24;
        u32 disp = (u32)(s32)x.snd;
        opcode[n++] = disp >> 0;
        opcode[n++] = disp >> 8;
        opcode[n++] = disp >> 16;
        opcode[n++] = disp >> 24;
        sljit_emit_op_custom(compiler, opcode, n);
        return;
      }
    }
#endif
    mov32_u8(scratch, y);
    or32(scratch, scratch, z);
    mov32_u8(x, scratch);
  }

  auto fsqrt32_f0() -> void {
#if defined(ARCHITECTURE_ARM64)
    u32 opcode = 0x1e21c000u;
    sljit_emit_op_custom(compiler, &opcode, sizeof(opcode));
#elif defined(ARCHITECTURE_AMD64)
    u8 opcode[] = {0xf3, 0x0f, 0x51, 0xc0};
    sljit_emit_op_custom(compiler, opcode, sizeof(opcode));
#endif
  }

  template<typename T, typename U>
  auto fabs32(T x, U y) -> void {
    sljit_emit_fop1(compiler, SLJIT_ABS_F32, x.fst, x.snd, y.fst, y.snd);
  }

  template<typename T, typename U>
  auto fneg32(T x, U y) -> void {
    sljit_emit_fop1(compiler, SLJIT_NEG_F32, x.fst, x.snd, y.fst, y.snd);
  }
//};
