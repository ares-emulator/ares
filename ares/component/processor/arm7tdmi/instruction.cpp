auto ARM7TDMI::fetch() -> void {
  pipeline.execute = pipeline.decode;
  pipeline.decode = pipeline.fetch;
  pipeline.decode.thumb = cpsr().t;

  u32 sequential = Sequential;
  if(pipeline.nonsequential) {
    pipeline.nonsequential = false;
    sequential = Nonsequential;
  }

  u32 mask = !cpsr().t ? 3 : 1;
  u32 size = !cpsr().t ? Word : Half;

  r(15).data += size >> 3;
  pipeline.fetch.address = r(15) & ~mask;
  pipeline.fetch.instruction = read(Prefetch | size | sequential, pipeline.fetch.address);
}

auto ARM7TDMI::instruction() -> void {
  u32 mask = !cpsr().t ? 3 : 1;
  u32 size = !cpsr().t ? Word : Half;

  if(pipeline.reload) {
    pipeline.reload = false;
    r(15).data &= ~mask;
    pipeline.fetch.address = r(15) & ~mask;
    pipeline.fetch.instruction = read(Prefetch | size | Nonsequential, pipeline.fetch.address);
    fetch();
  }

  n1 irqRaised = irq;
  fetch();

  if(irqRaised && !cpsr().i) {
    exception(PSR::IRQ, 0x18);
    if(pipeline.execute.thumb) r(14).data += 2;
    return;
  }

  opcode = pipeline.execute.instruction;
  if(!pipeline.execute.thumb) {
    if(!TST(opcode.bit(28,31))) return;
    n12 index = (opcode & 0x0ff00000) >> 16 | (opcode & 0x000000f0) >> 4;
    armInstruction[index](opcode);
  } else {
    thumbInstruction[(n16)opcode]();
  }
}

auto ARM7TDMI::exception(u32 mode, n32 address) -> void {
  auto psr = cpsr();
  cpsr().m = mode;
  spsr() = psr;
  cpsr().t = 0;
  if(cpsr().m == PSR::FIQ) cpsr().f = 1;
  cpsr().i = 1;
  r(14) = pipeline.decode.address;
  r(15) = address;
}

auto ARM7TDMI::armInitialize() -> void {
  #define bind(id, name, ...) { \
    u32 index = (id & 0x0ff00000) >> 16 | (id & 0x000000f0) >> 4; \
    assert(!armInstruction[index]); \
    armInstruction[index] = [&](n32 opcode) { return armInstruction##name(arguments); }; \
    armDisassemble[index] = [&](n32 opcode) { return armDisassemble##name(arguments); }; \
  }

  #define pattern(s) \
    std::integral_constant<u32, bit::test(s)>::value

  #define arguments \
    opcode.bit( 0,23),  /* displacement */ \
    opcode.bit(24)      /* link */
  for(n4 displacementLo : range(16))
  for(n4 displacementHi : range(16))
  for(n1 link : range(2)) {
    auto opcode = pattern(".... 101? ???? ???? ???? ???? ???? ????")
                | displacementLo << 4 | displacementHi << 20 | link << 24;
    bind(opcode, Branch);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3)   /* m */
  {
    auto opcode = pattern(".... 0001 0010 ---- ---- ---- 0001 ????");
    bind(opcode, BranchExchangeRegister);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 7),  /* immediate */ \
    opcode.bit( 8,11),  /* shift */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* save */ \
    opcode.bit(21,24)   /* mode */
  for(n4 shiftHi : range(16))
  for(n1 save : range(2))
  for(n4 mode : range(16)) {
    if(mode >= 8 && mode <= 11 && !save) continue;  //TST, TEQ, CMP, CMN
    auto opcode = pattern(".... 001? ???? ???? ???? ???? ???? ????") | shiftHi << 4 | save << 20 | mode << 21;
    bind(opcode, DataImmediate);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit( 5, 6),  /* type */ \
    opcode.bit( 7,11),  /* shift */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* save */ \
    opcode.bit(21,24)   /* mode */
  for(n2 type : range(4))
  for(n1 shiftLo : range(2))
  for(n1 save : range(2))
  for(n4 mode : range(16)) {
    if(mode >= 8 && mode <= 11 && !save) continue;  //TST, TEQ, CMP, CMN
    auto opcode = pattern(".... 000? ???? ???? ???? ???? ???0 ????") | type << 5 | shiftLo << 7 | save << 20 | mode << 21;
    bind(opcode, DataImmediateShift);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit( 5, 6),  /* type */ \
    opcode.bit( 8,11),  /* s */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* save */ \
    opcode.bit(21,24)   /* mode */
  for(n2 type : range(4))
  for(n1 save : range(2))
  for(n4 mode : range(16)) {
    if(mode >= 8 && mode <= 11 && !save) continue;  //TST, TEQ, CMP, CMN
    auto opcode = pattern(".... 000? ???? ???? ???? ???? 0??1 ????") | type << 5 | save << 20 | mode << 21;
    bind(opcode, DataRegisterShift);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3) << 0 | opcode.bit( 8,11) << 4,  /* immediate */ \
    opcode.bit( 5),     /* half */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n1 half : range(2))
  for(n1 writeback : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 000? ?1?1 ???? ???? ???? 11?1 ????") | half << 5 | writeback << 21 | up << 23 | pre << 24;
    bind(opcode, LoadImmediate);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit( 5),     /* half */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n1 half : range(2))
  for(n1 writeback : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 000? ?0?1 ???? ???? ---- 11?1 ????") | half << 5 | writeback << 21 | up << 23 | pre << 24;
    bind(opcode, LoadRegister);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(22)      /* byte */
  for(n1 byte : range(2)) {
    auto opcode = pattern(".... 0001 0?00 ???? ???? ---- 1001 ????") | byte << 22;
    bind(opcode, MemorySwap);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3) << 0 | opcode.bit( 8,11) << 4,  /* immediate */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* mode */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n1 mode : range(2))
  for(n1 writeback : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 000? ?1?? ???? ???? ???? 1011 ????") | mode << 20 | writeback << 21 | up << 23 | pre << 24;
    bind(opcode, MoveHalfImmediate);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* mode */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n1 mode : range(2))
  for(n1 writeback : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 000? ?0?? ???? ???? ---- 1011 ????") | mode << 20 | writeback << 21 | up << 23 | pre << 24;
    bind(opcode, MoveHalfRegister);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0,11),  /* immediate */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* mode */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(22),     /* byte */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n4 immediatePart : range(16))
  for(n1 mode : range(2))
  for(n1 writeback : range(2))
  for(n1 byte : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 010? ???? ???? ???? ???? ???? ????")
                | immediatePart << 4 | mode << 20 | writeback << 21 | byte << 22 | up << 23 | pre << 24;
    bind(opcode, MoveImmediateOffset);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0,15),  /* list */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* mode */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(22),     /* type */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n4 listPart : range(16))
  for(n1 mode : range(2))
  for(n1 writeback : range(2))
  for(n1 type : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 100? ???? ???? ???? ???? ???? ????")
                | listPart << 4 | mode << 20 | writeback << 21 | type << 22 | up << 23 | pre << 24;
    bind(opcode, MoveMultiple);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit( 5, 6),  /* type */ \
    opcode.bit( 7,11),  /* shift */ \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(16,19),  /* n */ \
    opcode.bit(20),     /* mode */ \
    opcode.bit(21),     /* writeback */ \
    opcode.bit(22),     /* byte */ \
    opcode.bit(23),     /* up */ \
    opcode.bit(24)      /* pre */
  for(n2 type : range(4))
  for(n1 shiftLo : range(2))
  for(n1 mode : range(2))
  for(n1 writeback : range(2))
  for(n1 byte : range(2))
  for(n1 up : range(2))
  for(n1 pre : range(2)) {
    auto opcode = pattern(".... 011? ???? ???? ???? ???? ???0 ????")
                | type << 5 | shiftLo << 7 | mode << 20 | writeback << 21 | byte << 22 | up << 23 | pre << 24;
    bind(opcode, MoveRegisterOffset);
  }
  #undef arguments

  #define arguments \
    opcode.bit(12,15),  /* d */ \
    opcode.bit(22)      /* mode */
  for(n1 mode : range(2)) {
    auto opcode = pattern(".... 0001 0?00 ---- ???? ---- 0000 ----") | mode << 22;
    bind(opcode, MoveToRegisterFromStatus);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 7),  /* immediate */ \
    opcode.bit( 8,11),  /* rotate */ \
    opcode.bit(16,19),  /* field */ \
    opcode.bit(22)      /* mode */
  for(n4 immediateHi : range(16))
  for(n1 mode : range(2)) {
    auto opcode = pattern(".... 0011 0?10 ???? ---- ???? ???? ????") | immediateHi << 4 | mode << 22;
    bind(opcode, MoveToStatusFromImmediate);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit(16,19),  /* field */ \
    opcode.bit(22)      /* mode */
  for(n1 mode : range(2)) {
    auto opcode = pattern(".... 0001 0?10 ???? ---- ---- 0000 ????") | mode << 22;
    bind(opcode, MoveToStatusFromRegister);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit( 8,11),  /* s */ \
    opcode.bit(12,15),  /* n */ \
    opcode.bit(16,19),  /* d */ \
    opcode.bit(20),     /* save */ \
    opcode.bit(21)      /* accumulate */
  for(n1 save : range(2))
  for(n1 accumulate : range(2)) {
    auto opcode = pattern(".... 0000 00?? ???? ???? ???? 1001 ????") | save << 20 | accumulate << 21;
    bind(opcode, Multiply);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0, 3),  /* m */ \
    opcode.bit( 8,11),  /* s */ \
    opcode.bit(12,15),  /* l */ \
    opcode.bit(16,19),  /* h */ \
    opcode.bit(20),     /* save */ \
    opcode.bit(21),     /* accumulate */ \
    opcode.bit(22)      /* sign */
  for(n1 save : range(2))
  for(n1 accumulate : range(2))
  for(n1 sign : range(2)) {
    auto opcode = pattern(".... 0000 1??? ???? ???? ???? 1001 ????") | save << 20 | accumulate << 21 | sign << 22;
    bind(opcode, MultiplyLong);
  }
  #undef arguments

  #define arguments \
    opcode.bit( 0,23)  /* immediate */
  for(n4 immediateLo : range(16))
  for(n4 immediateHi : range(16)) {
    auto opcode = pattern(".... 1111 ???? ???? ???? ???? ???? ????") | immediateLo << 4 | immediateHi << 20;
    bind(opcode, SoftwareInterrupt);
  }
  #undef arguments

  #define arguments
  for(n12 id : range(4096)) {
    if(armInstruction[id]) continue;
    auto opcode = pattern(".... ???? ???? ---- ---- ---- ???? ----") | id.bit(0,3) << 4 | id.bit(4,11) << 20;
    bind(opcode, Undefined);
  }
  #undef arguments

  #undef bind
  #undef pattern
}

auto ARM7TDMI::thumbInitialize() -> void {
  #define bind(id, name, ...) { \
    assert(!thumbInstruction[id]); \
    thumbInstruction[id] = [=] { return thumbInstruction##name(__VA_ARGS__); }; \
    thumbDisassemble[id] = [=] { return thumbDisassemble##name(__VA_ARGS__); }; \
  }

  #define pattern(s) \
    std::integral_constant<u16, bit::test(s)>::value

  for(n3 d : range(8))
  for(n3 m : range(8))
  for(n4 mode : range(16)) {
    auto opcode = pattern("0100 00?? ???? ????") | d << 0 | m << 3 | mode << 6;
    bind(opcode, ALU, d, m, mode);
  }

  for(n4 d : range(16))
  for(n4 m : range(16))
  for(n2 mode : range(4)) {
    if(mode == 3) continue;
    auto opcode = pattern("0100 01?? ???? ????") | d.bit(0,2) << 0 | m << 3 | d.bit(3) << 7 | mode << 8;
    bind(opcode, ALUExtended, d, m, mode);
  }

  for(n8 immediate : range(256))
  for(n3 d : range(8))
  for(n1 mode : range(2)) {
    auto opcode = pattern("1010 ???? ???? ????") | immediate << 0 | d << 8 | mode << 11;
    bind(opcode, AddRegister, immediate, d, mode);
  }

  for(n3 d : range(8))
  for(n3 n : range(8))
  for(n3 immediate : range(8))
  for(n1 mode : range(2)) {
    auto opcode = pattern("0001 11?? ???? ????") | d << 0 | n << 3 | immediate << 6 | mode << 9;
    bind(opcode, AdjustImmediate, d, n, immediate, mode);
  }

  for(n3 d : range(8))
  for(n3 n : range(8))
  for(n3 m : range(8))
  for(n1 mode : range(2)) {
    auto opcode = pattern("0001 10?? ???? ????") | d << 0 | n << 3 | m << 6 | mode << 9;
    bind(opcode, AdjustRegister, d, n, m, mode);
  }

  for(n7 immediate : range(128))
  for(n1 mode : range(2)) {
    auto opcode = pattern("1011 0000 ???? ????") | immediate << 0 | mode << 7;
    bind(opcode, AdjustStack, immediate, mode);
  }

  for(n3 _ : range(8))
  for(n4 m : range(16)) {
    auto opcode = pattern("0100 0111 0??? ?---") | _ << 0 | m << 3;
    bind(opcode, BranchExchange, m);
  }

  for(n11 displacement : range(2048)) {
    auto opcode = pattern("1111 0??? ???? ????") | displacement << 0;
    bind(opcode, BranchFarPrefix, displacement);
  }

  for(n11 displacement : range(2048)) {
    auto opcode = pattern("1111 1??? ???? ????") | displacement << 0;
    bind(opcode, BranchFarSuffix, displacement);
  }

  for(n11 displacement : range(2048)) {
    auto opcode = pattern("1110 0??? ???? ????") | displacement << 0;
    bind(opcode, BranchNear, displacement);
  }

  for(n8 displacement : range(256))
  for(n4 condition : range(16)) {
    if(condition == 15) continue;  //BNV
    auto opcode = pattern("1101 ???? ???? ????") | displacement << 0 | condition << 8;
    bind(opcode, BranchTest, displacement, condition);
  }

  for(n8 immediate : range(256))
  for(n3 d : range(8))
  for(n2 mode : range(4)) {
    auto opcode = pattern("001? ???? ???? ????") | immediate << 0 | d << 8 | mode << 11;
    bind(opcode, Immediate, immediate, d, mode);
  }

  for(n8 displacement : range(256))
  for(n3 d : range(8)) {
    auto opcode = pattern("0100 1??? ???? ????") | displacement << 0 | d << 8;
    bind(opcode, LoadLiteral, displacement, d);
  }

  for(n3 d : range(8))
  for(n3 n : range(8))
  for(n5 immediate : range(32))
  for(n1 mode : range(2)) {
    auto opcode = pattern("0111 ???? ???? ????") | d << 0 | n << 3 | immediate << 6 | mode << 11;
    bind(opcode, MoveByteImmediate, d, n, immediate, mode);
  }

  for(n3 d : range(8))
  for(n3 n : range(8))
  for(n5 immediate : range(32))
  for(n1 mode : range(2)) {
    auto opcode = pattern("1000 ???? ???? ????") | d << 0 | n << 3 | immediate << 6 | mode << 11;
    bind(opcode, MoveHalfImmediate, d, n, immediate, mode);
  }

  for(n8 list : range(256))
  for(n3 n : range(8))
  for(n1 mode : range(2)) {
    auto opcode = pattern("1100 ???? ???? ????") | list << 0 | n << 8 | mode << 11;
    bind(opcode, MoveMultiple, list, n, mode);
  }

  for(n3 d : range(8))
  for(n3 n : range(8))
  for(n3 m : range(8))
  for(n3 mode : range(8)) {
    auto opcode = pattern("0101 ???? ???? ????") | d << 0 | n << 3 | m << 6 | mode << 9;
    bind(opcode, MoveRegisterOffset, d, n, m, mode);
  }

  for(n8 immediate : range(256))
  for(n3 d : range(8))
  for(n1 mode : range(2)) {
    auto opcode = pattern("1001 ???? ???? ????") | immediate << 0 | d << 8 | mode << 11;
    bind(opcode, MoveStack, immediate, d, mode);
  }

  for(n3 d : range(8))
  for(n3 n : range(8))
  for(n5 offset : range(32))
  for(n1 mode : range(2)) {
    auto opcode = pattern("0110 ???? ???? ????") | d << 0 | n << 3 | offset << 6 | mode << 11;
    bind(opcode, MoveWordImmediate, d, n, offset, mode);
  }

  for(n3 d : range(8))
  for(n3 m : range(8))
  for(n5 immediate : range(32))
  for(n2 mode : range(4)) {
    if(mode == 3) continue;
    auto opcode = pattern("000? ???? ???? ????") | d << 0 | m << 3 | immediate << 6 | mode << 11;
    bind(opcode, ShiftImmediate, d, m, immediate, mode);
  }

  for(n8 immediate : range(256)) {
    auto opcode = pattern("1101 1111 ???? ????") | immediate << 0;
    bind(opcode, SoftwareInterrupt, immediate);
  }

  for(n8 list : range(256))
  for(n1 lrpc : range(2))
  for(n1 mode : range(2)) {
    auto opcode = pattern("1011 ?10? ???? ????") | list << 0 | lrpc << 8 | mode << 11;
    bind(opcode, StackMultiple, list, lrpc, mode);
  }

  for(n16 id : range(65536)) {
    if(thumbInstruction[id]) continue;
    auto opcode = pattern("???? ???? ???? ????") | id << 0;
    bind(opcode, Undefined);
  }

  #undef bind
  #undef pattern
}
