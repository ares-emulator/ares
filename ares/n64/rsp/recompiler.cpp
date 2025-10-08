auto RSP::Recompiler::measure(u12 address) -> u12 {
  u12 start = address;
  bool hasBranched = 0;
  while(true) {
    u32 instruction = self.imem.read<Word>(address);
    bool branched = isTerminal(instruction);
    address += 4;
    if(hasBranched || address == start) break;
    hasBranched = branched;
  }

  return address - start;
}

auto RSP::Recompiler::hash(u12 address, u12 size) -> u64 {
  u12 end = address + size;
  if(address < end) {
    return XXH3_64bits(self.imem.data + address, size);
  } else {
    return XXH3_64bits(self.imem.data + address, self.imem.size - address)
         ^ XXH3_64bits(self.imem.data, end);
  }
}

auto RSP::Recompiler::block(u12 address) -> Block* {
  if(dirty) {
    u12 address = 0;
    for(auto& block : context) {
      if(block && (dirty & mask(address, block->size)) != 0) {
        block = nullptr;
      }
      address += 4;
    }
    dirty = 0;
  }

  if(auto block = context[address >> 2]) return block;

  auto size = measure(address);
  auto hashcode = hash(address, size);
  hashcode ^= self.pipeline.hash();

  auto it = blocks.find(hashcode);
  if(it != blocks.end()) return context[address >> 2] = it->second;

  auto block = emit(address);
  assert(block->size == size);
  memory::jitprotect(true);

  blocks.emplace(hashcode, block);
  return context[address >> 2] = block;

  throw;  //should never occur
}

#define IpuReg(r) sreg(1), offsetof(IPU, r)
#define VuReg(r)  sreg(2), offsetof(VU, r)
#define R0        IpuReg(r[0])

auto RSP::Recompiler::emit(u12 address) -> Block* {
  if(unlikely(allocator.available() < 128_KiB)) {
    print("RSP allocator flush\n");
    allocator.release();
    reset();
  }

  pipeline = self.pipeline;

  auto block = (Block*)allocator.acquire(sizeof(Block));
  beginFunction(3);

  u12 start = address;
  bool hasBranched = 0;
  while(true) {
    u32 instruction = self.imem.read<Word>(address);
    if(callInstructionPrologue) {
      callf(&RSP::instructionPrologue, imm(instruction));
    }
    pipeline.begin();
    OpInfo op0 = self.decoderEXECUTE(instruction);
    pipeline.issue(op0);
    bool branched = emitEXECUTE(instruction);
    if(op0.r.def & 1) mov32(mem(R0), imm(0));

    if(!pipeline.singleIssue && !branched && u12(address + 4) != start) {
      u32 instruction = self.imem.read<Word>(address + 4);
      OpInfo op1 = self.decoderEXECUTE(instruction);

      if(RSP::canDualIssue(op0, op1)) {
        callf(&RSP::instructionEpilogue<1>, imm(0));
        if(callInstructionPrologue) {
          callf(&RSP::instructionPrologue, imm(instruction));
        }
        address += 4;
        pipeline.issue(op1);
        branched = emitEXECUTE(instruction);
        if(op1.r.def & 1) mov32(mem(R0), imm(0));
      }
    }

    pipeline.end();
    callf(&RSP::instructionEpilogue<1>, imm(pipeline.clocks));
    address += 4;
    if(hasBranched || address == start) break;
    hasBranched = branched;
    testJumpEpilog();
  }
  jumpEpilog();

  //reset clocks to zero every time block is executed
  pipeline.clocks = 0;

  memory::jitprotect(false);
  block->code = endFunction();
  block->size = address - start;
  block->pipeline = pipeline;

//print(hex(PC, 8L), " ", instructions, " ", size(), "\n");
  return block;
}

#define Sa  (instruction >>  6 & 31)
#define Rdn (instruction >> 11 & 31)
#define Rtn (instruction >> 16 & 31)
#define Rsn (instruction >> 21 & 31)
#define Vdn (instruction >>  6 & 31)
#define Vsn (instruction >> 11 & 31)
#define Vtn (instruction >> 16 & 31)
#define Rd  IpuReg(r[0]) + Rdn * sizeof(r32)
#define Rt  IpuReg(r[0]) + Rtn * sizeof(r32)
#define Rs  IpuReg(r[0]) + Rsn * sizeof(r32)
#define Vd  VuReg(r[0]) + Vdn * sizeof(r128)
#define Vs  VuReg(r[0]) + Vsn * sizeof(r128)
#define Vt  VuReg(r[0]) + Vtn * sizeof(r128)
#define i16 s16(instruction)
#define n16 u16(instruction)
#define n26 u32(instruction & 0x03ff'ffff)

#define callvu(name,...) \
  switch(E) { \
  case 0x0: callf(name<0x0>, __VA_ARGS__); break; \
  case 0x1: callf(name<0x1>, __VA_ARGS__); break; \
  case 0x2: callf(name<0x2>, __VA_ARGS__); break; \
  case 0x3: callf(name<0x3>, __VA_ARGS__); break; \
  case 0x4: callf(name<0x4>, __VA_ARGS__); break; \
  case 0x5: callf(name<0x5>, __VA_ARGS__); break; \
  case 0x6: callf(name<0x6>, __VA_ARGS__); break; \
  case 0x7: callf(name<0x7>, __VA_ARGS__); break; \
  case 0x8: callf(name<0x8>, __VA_ARGS__); break; \
  case 0x9: callf(name<0x9>, __VA_ARGS__); break; \
  case 0xa: callf(name<0xa>, __VA_ARGS__); break; \
  case 0xb: callf(name<0xb>, __VA_ARGS__); break; \
  case 0xc: callf(name<0xc>, __VA_ARGS__); break; \
  case 0xd: callf(name<0xd>, __VA_ARGS__); break; \
  case 0xe: callf(name<0xe>, __VA_ARGS__); break; \
  case 0xf: callf(name<0xf>, __VA_ARGS__); break; \
  }

auto RSP::Recompiler::emitEXECUTE(u32 instruction) -> bool {
  switch(instruction >> 26) {

  //SPECIAL
  case 0x00: {
    return emitSPECIAL(instruction);
  }

  //REGIMM
  case 0x01: {
    return emitREGIMM(instruction);
  }

  //J n26
  case 0x02: {
    callf(&RSP::J, imm(n26));
    return 1;
  }

  //JAL n26
  case 0x03: {
    callf(&RSP::JAL, imm(n26));
    return 1;
  }

  //BEQ Rs,Rt,i16
  case 0x04: {
    callf(&RSP::BEQ, mem(Rs), mem(Rt), imm(i16));
    return 1;
  }

  //BNE Rs,Rt,i16
  case 0x05: {
    callf(&RSP::BNE, mem(Rs), mem(Rt), imm(i16));
    return 1;
  }

  //BLEZ Rs,i16
  case 0x06: {
    callf(&RSP::BLEZ, mem(Rs), imm(i16));
    return 1;
  }

  //BGTZ Rs,i16
  case 0x07: {
    callf(&RSP::BGTZ, mem(Rs), imm(i16));
    return 1;
  }

  //ADDIU Rt,Rs,i16
  case range2(0x08, 0x09): {
    add32(mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SLTI Rt,Rs,i16
  case 0x0a: {
    cmp32(mem(Rs), imm(i16), set_slt);
    mov32_f(mem(Rt), flag_slt);
    return 0;
  }

  //SLTIU Rt,Rs,i16
  case 0x0b: {
    cmp32(mem(Rs), imm(i16), set_ult);
    mov32_f(mem(Rt), flag_ult);
    return 0;
  }

  //ANDI Rt,Rs,n16
  case 0x0c: {
    and32(mem(Rt), mem(Rs), imm(n16));
    return 0;
  }

  //ORI Rt,Rs,n16
  case 0x0d: {
    or32(mem(Rt), mem(Rs), imm(n16));
    return 0;
  }

  //XORI Rt,Rs,n16
  case 0x0e: {
    xor32(mem(Rt), mem(Rs), imm(n16));
    return 0;
  }

  //LUI Rt,n16
  case 0x0f: {
    mov32(mem(Rt), imm(s32(n16 << 16)));
    return 0;
  }

  //SCC
  case 0x10: {
    return emitSCC(instruction);
  }

  //INVALID
  case 0x11: {
    return 0;
  }

  //VPU
  case 0x12: {
    return emitVU(instruction);
  }

  //INVALID
  case range13(0x13, 0x1f): {
    return 0;
  }

  //LB Rt,Rs,i16
  case 0x20: {
    callf(&RSP::LB, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //LH Rt,Rs,i16
  case 0x21: {
    callf(&RSP::LH, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case 0x22: {
    return 0;
  }

  //LW Rt,Rs,i16
  case 0x23: {
    callf(&RSP::LW, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //LBU Rt,Rs,i16
  case 0x24: {
    callf(&RSP::LBU, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //LHU Rt,Rs,i16
  case 0x25: {
    callf(&RSP::LHU, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case 0x26: {
    return 0;
  }

  //LWU Rt,Rs,i16
  case 0x27: {
    callf(&RSP::LWU, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SB Rt,Rs,i16
  case 0x28: {
    callf(&RSP::SB, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //SH Rt,Rs,i16
  case 0x29: {
    callf(&RSP::SH, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case 0x2a: {
    return 0;
  }

  //SW Rt,Rs,i16
  case 0x2b: {
    callf(&RSP::SW, mem(Rt), mem(Rs), imm(i16));
    return 0;
  }

  //INVALID
  case range6(0x2c, 0x31): {
    return 0;
  }

  //LWC2
  case 0x32: {
    return emitLWC2(instruction);
  }

  //INVALID
  case range7(0x33, 0x39): {
    return 0;
  }

  //SWC2
  case 0x3a: {
    return emitSWC2(instruction);
  }

  //INVALID
  case range5(0x3b, 0x3f): {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitSPECIAL(u32 instruction) -> bool {
  switch(instruction & 0x3f) {

  //SLL Rd,Rt,Sa
  case 0x00: {
    shl32(mem(Rd), mem(Rt), imm(Sa));
    return 0;
  }

  //INVALID
  case 0x01: {
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return 0;
  }

  //SRL Rd,Rt,Sa
  case 0x02: {
    lshr32(mem(Rd), mem(Rt), imm(Sa));
    return 0;
  }

  //SRA Rd,Rt,Sa
  case 0x03: {
    ashr32(mem(Rd), mem(Rt), imm(Sa));
    return 0;
  }

  //SLLV Rd,Rt,Rs
  case 0x04: {
    mshl32(mem(Rd), mem(Rt), mem(Rs));
    return 0;
  }

  //INVALID
  case 0x05: {
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return 0;
  }

  //SRLV Rd,Rt,Rs
  case 0x06: {
    mlshr32(mem(Rd), mem(Rt), mem(Rs));
    return 0;
  }

  //SRAV Rd,Rt,Rs
  case 0x07: {
    mashr32(mem(Rd), mem(Rt), mem(Rs));
    return 0;
  }

  //JR Rs
  case 0x08: {
    callf(&RSP::JR, mem(Rs));
    return 1;
  }

  //JALR Rd,Rs
  case 0x09: {
    callf(&RSP::JALR, mem(Rd), mem(Rs));
    return 1;
  }

  //INVALID
  case range3(0x0a, 0x0c): {
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return 0;
  }

  //BREAK
  case 0x0d: {
    callf(&RSP::BREAK);
    return 1;
  }

  //INVALID
  case range18(0x0e, 0x1f): {
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return 0;
  }

  //ADDU Rd,Rs,Rt
  case range2(0x20, 0x21): {
    add32(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //SUBU Rd,Rs,Rt
  case range2(0x22, 0x23): {
    sub32(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //AND Rd,Rs,Rt
  case 0x24: {
    and32(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //OR Rd,Rs,Rt
  case 0x25: {
    or32(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //XOR Rd,Rs,Rt
  case 0x26: {
    xor32(mem(Rd), mem(Rs), mem(Rt));
    return 0;
  }

  //NOR Rd,Rs,Rt
  case 0x27: {
    or32(reg(0), mem(Rs), mem(Rt));
    xor32(reg(0), reg(0), imm(-1));
    mov32(mem(Rd), reg(0));
    return 0;
  }

  //INVALID
  case range2(0x28, 0x29): {
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return 0;
  }

  //SLT Rd,Rs,Rt
  case 0x2a: {
    cmp32(mem(Rs), mem(Rt), set_slt);
    mov32_f(mem(Rd), flag_slt);
    return 0;
  }

  //SLTU Rd,Rs,Rt
  case 0x2b: {
    cmp32(mem(Rs), mem(Rt), set_ult);
    mov32_f(mem(Rd), flag_ult);
    return 0;
  }

  //INVALID
  case range20(0x2c, 0x3f): {
    mlshr32(mem(Rd), mem(Rs), mem(Rs));
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitREGIMM(u32 instruction) -> bool {
  switch(instruction >> 16 & 0x1f) {

  //BLTZ Rs,i16
  case 0x00: {
    callf(&RSP::BLTZ, mem(Rs), imm(i16));
    return 1;
  }

  //BGEZ Rs,i16
  case 0x01: {
    callf(&RSP::BGEZ, mem(Rs), imm(i16));
    return 1;
  }

  //INVALID
  case range14(0x02, 0x0f): {
    return 0;
  }

  //BLTZAL Rs,i16
  case 0x10: {
    callf(&RSP::BLTZAL, mem(Rs), imm(i16));
    return 1;
  }

  //BGEZAL Rs,i16
  case 0x11: {
    callf(&RSP::BGEZAL, mem(Rs), imm(i16));
    return 1;
  }

  //INVALID
  case range14(0x12, 0x1f): {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitSCC(u32 instruction) -> bool {
  switch(instruction >> 21 & 0x1f) {

  //MFC0 Rt,Rd
  case 0x00: {
    callf(&RSP::MFC0, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range3(0x01, 0x03): {
    return 0;
  }

  //MTC0 Rt,Rd
  case 0x04: {
    callf(&RSP::MTC0, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range27(0x05, 0x1f): {
    return 0;
  }

  }

  return 0;
}

auto RSP::Recompiler::emitVU(u32 instruction) -> bool {
  #define E (instruction >> 7 & 15)
  switch(instruction >> 21 & 0x1f) {

  //MFC2 Rt,Vs(e)
  case 0x00: {
    callvu(&RSP::MFC2, mem(Rt), mem(Vs));
    return 0;
  }

  //INVALID
  case 0x01: {
    return 0;
  }

  //CFC2 Rt,Rd
  case 0x02: {
    callf(&RSP::CFC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case 0x03: {
    return 0;
  }

  //MTC2 Rt,Vs(e)
  case 0x04: {
    callvu(&RSP::MTC2, mem(Rt), mem(Vs));
    return 0;
  }

  //INVALID
  case 0x05: {
    return 0;
  }

  //CTC2 Rt,Rd
  case 0x06: {
    callf(&RSP::CTC2, mem(Rt), imm(Rdn));
    return 0;
  }

  //INVALID
  case range9(0x07, 0x0f): {
    return 0;
  }

  }
  #undef E

  #define E  (instruction >> 21 & 15)
  #define DE (instruction >> 11 &  7)
  switch(instruction & 0x3f) {

  //VMULF Vd,Vs,Vt(e)
  case 0x00: {
    callvu(&RSP::VMULF, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMULU Vd,Vs,Vt(e)
  case 0x01: {
    callvu(&RSP::VMULU, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VRNDP Vd,Vs,Vt(e)
  case 0x02: {
    callvu(&RSP::VRNDP, mem(Vd), imm(Vsn), mem(Vt));
    return 0;
  }

  //VMULQ Vd,Vs,Vt(e)
  case 0x03: {
    callvu(&RSP::VMULQ, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMUDL Vd,Vs,Vt(e)
  case 0x04: {
    callvu(&RSP::VMUDL, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMUDM Vd,Vs,Vt(e)
  case 0x05: {
    callvu(&RSP::VMUDM, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMUDN Vd,Vs,Vt(e)
  case 0x06: {
    callvu(&RSP::VMUDN, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMUDH Vd,Vs,Vt(e)
  case 0x07: {
    callvu(&RSP::VMUDH, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMACF Vd,Vs,Vt(e)
  case 0x08: {
    callvu(&RSP::VMACF, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMACU Vd,Vs,Vt(e)
  case 0x09: {
    callvu(&RSP::VMACU, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VRNDN Vd,Vs,Vt(e)
  case 0x0a: {
    callvu(&RSP::VRNDN, mem(Vd), imm(Vsn), mem(Vt));
    return 0;
  }

  //VMACQ Vd
  case 0x0b: {
    callf(&RSP::VMACQ, mem(Vd));
    return 0;
  }

  //VMADL Vd,Vs,Vt(e)
  case 0x0c: {
    callvu(&RSP::VMADL, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMADM Vd,Vs,Vt(e)
  case 0x0d: {
    callvu(&RSP::VMADM, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMADN Vd,Vs,Vt(e)
  case 0x0e: {
    callvu(&RSP::VMADN, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMADH Vd,Vs,Vt(e)
  case 0x0f: {
    callvu(&RSP::VMADH, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VADD Vd,Vs,Vt(e)
  case 0x10: {
    callvu(&RSP::VADD, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VSUB Vd,Vs,Vt(e)
  case 0x11: {
    callvu(&RSP::VSUB, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VSUT (broken)
  case 0x12: {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VABS Vd,Vs,Vt(e)
  case 0x13: {
    callvu(&RSP::VABS, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VADDC Vd,Vs,Vt(e)
  case 0x14: {
    callvu(&RSP::VADDC, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VSUBC Vd,Vs,Vt(e)
  case 0x15: {
    callvu(&RSP::VSUBC, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //Broken opcodes: VADDB, VSUBB, VACCB, VSUCB, VSAD, VSAC, VSUM
  case range7(0x16, 0x1c): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VSAR Vd,Vs,E
  case 0x1d: {
    callvu(&RSP::VSAR, mem(Vd), mem(Vs));
    return 0;
  }

  //Invalid opcodes
  case range2(0x1e, 0x1f): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VLT Vd,Vs,Vt(e)
  case 0x20: {
    callvu(&RSP::VLT, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VEQ Vd,Vs,Vt(e)
  case 0x21: {
    callvu(&RSP::VEQ, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VNE Vd,Vs,Vt(e)
  case 0x22: {
    callvu(&RSP::VNE, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VGE Vd,Vs,Vt(e)
  case 0x23: {
    callvu(&RSP::VGE, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VCL Vd,Vs,Vt(e)
  case 0x24: {
    callvu(&RSP::VCL, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VCH Vd,Vs,Vt(e)
  case 0x25: {
    callvu(&RSP::VCH, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VCR Vd,Vs,Vt(e)
  case 0x26: {
    callvu(&RSP::VCR, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VMRG Vd,Vs,Vt(e)
  case 0x27: {
    callvu(&RSP::VMRG, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VAND Vd,Vs,Vt(e)
  case 0x28: {
    callvu(&RSP::VAND, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VNAND Vd,Vs,Vt(e)
  case 0x29: {
    callvu(&RSP::VNAND, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VOR Vd,Vs,Vt(e)
  case 0x2a: {
    callvu(&RSP::VOR, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VNOR Vd,Vs,Vt(e)
  case 0x2b: {
    callvu(&RSP::VNOR, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VXOR Vd,Vs,Vt(e)
  case 0x2c: {
    callvu(&RSP::VXOR, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VNXOR Vd,Vs,Vt(e)
  case 0x2d: {
    callvu(&RSP::VNXOR, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //INVALID
  case range2(0x2e, 0x2f): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //VCRP Vd(de),Vt(e)
  case 0x30: {
    callvu(&RSP::VRCP, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VRCPL Vd(de),Vt(e)
  case 0x31: {
    callvu(&RSP::VRCPL, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VRCPH Vd(de),Vt(e)
  case 0x32: {
    callvu(&RSP::VRCPH, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VMOV Vd(de),Vt(e)
  case 0x33: {
    callvu(&RSP::VMOV, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VRSQ Vd(de),Vt(e)
  case 0x34: {
    callvu(&RSP::VRSQ, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VRSQL Vd(de),Vt(e)
  case 0x35: {
    callvu(&RSP::VRSQL, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VRSQH Vd(de),Vt(e)
  case 0x36: {
    callvu(&RSP::VRSQH, mem(Vd), imm(DE), mem(Vt));
    return 0;
  }

  //VNOP
  case 0x37: {
    callf(&RSP::VNOP);
    return 0;
  }
//Broken opcodes: VEXTT, VEXTQ, VEXTN
  case range3(0x38, 0x3a): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;        
  }

  //INVALID
  case 0x3b: {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;
  }

  //Broken opcodes: VINST, VINSQ, VINSN
  case range3(0x3c, 0x3e): {
    callvu(&RSP::VZERO, mem(Vd), mem(Vs), mem(Vt));
    return 0;        
  }

  //VNULL
  case 0x3f: {
    callf(&RSP::VNOP);
    return 0;
  }

  }
  #undef E
  #undef DE

  return 0;
}

auto RSP::Recompiler::emitLWC2(u32 instruction) -> bool {
  #define E  (instruction >> 7 & 15)
  #define i7 (s8(instruction << 1) >> 1)
  switch(instruction >> 11 & 0x1f) {

  //LBV Vt(e),Rs,i7
  case 0x00: {
    callvu(&RSP::LBV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LSV Vt(e),Rs,i7
  case 0x01: {
    callvu(&RSP::LSV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LLV Vt(e),Rs,i7
  case 0x02: {
    callvu(&RSP::LLV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LDV Vt(e),Rs,i7
  case 0x03: {
    callvu(&RSP::LDV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LQV Vt(e),Rs,i7
  case 0x04: {
    callvu(&RSP::LQV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LRV Vt(e),Rs,i7
  case 0x05: {
    callvu(&RSP::LRV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LPV Vt(e),Rs,i7
  case 0x06: {
    callvu(&RSP::LPV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LUV Vt(e),Rs,i7
  case 0x07: {
    callvu(&RSP::LUV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LHV Vt(e),Rs,i7
  case 0x08: {
    callvu(&RSP::LHV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LFV Vt(e),Rs,i7
  case 0x09: {
    callvu(&RSP::LFV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //LWV (not present on N64 RSP)
  case 0x0a: {
    return 0;
  }

  //LTV Vt(e),Rs,i7
  case 0x0b: {
    callvu(&RSP::LTV, imm(Vtn), mem(Rs), imm(i7));
    return 0;
  }

  //INVALID
  case range20(0x0c, 0x1f): {
    return 0;
  }
  }
  #undef E
  #undef i7

  return 0;
}

auto RSP::Recompiler::emitSWC2(u32 instruction) -> bool {
  #define E  (instruction >> 7 & 15)
  #define i7 (s8(instruction << 1) >> 1)
  switch(instruction >> 11 & 0x1f) {

  //SBV Vt(e),Rs,i7
  case 0x00: {
    callvu(&RSP::SBV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SSV Vt(e),Rs,i7
  case 0x01: {
    callvu(&RSP::SSV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SLV Vt(e),Rs,i7
  case 0x02: {
    callvu(&RSP::SLV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SDV Vt(e),Rs,i7
  case 0x03: {
    callvu(&RSP::SDV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SQV Vt(e),Rs,i7
  case 0x04: {
    callvu(&RSP::SQV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SRV Vt(e),Rs,i7
  case 0x05: {
    callvu(&RSP::SRV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SPV Vt(e),Rs,i7
  case 0x06: {
    callvu(&RSP::SPV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SUV Vt(e),Rs,i7
  case 0x07: {
    callvu(&RSP::SUV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SHV Vt(e),Rs,i7
  case 0x08: {
    callvu(&RSP::SHV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SFV Vt(e),Rs,i7
  case 0x09: {
    callvu(&RSP::SFV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //SWV Vt(e),Rs,i7
  case 0x0a: {
    callvu(&RSP::SWV, mem(Vt), mem(Rs), imm(i7));
    return 0;
  }

  //STV Vt(e),Rs,i7
  case 0x0b: {
    callvu(&RSP::STV, imm(Vtn), mem(Rs), imm(i7));
    return 0;
  }
  //INVALID
  case range20(0x0c, 0x1f): {
    return 0;
  }

  }
  #undef E
  #undef i7

  return 0;
}

auto RSP::Recompiler::isTerminal(u32 instruction) -> bool {
  switch(instruction >> 26) {

  //SPECIAL
  case 0x00: {
    switch(instruction & 0x3f) {

    //JR Rs
    case 0x08:
    //JALR Rd,Rs
    case 0x09:
    //BREAK
    case 0x0d:
      return 1;

    }

    break;
  }

  //REGIMM
  case 0x01: {
    switch(instruction >> 16 & 0x1f) {

    //BLTZ Rs,i16
    case 0x00:
    //BGEZ Rs,i16
    case 0x01:
    //BLTZAL Rs,i16
    case 0x10:
    //BGEZAL Rs,i16
    case 0x11:
      return 1;

    }

    break;
  }

  //J n26
  case 0x02:
  //JAL n26
  case 0x03:
  //BEQ Rs,Rt,i16
  case 0x04:
  //BNE Rs,Rt,i16
  case 0x05:
  //BLEZ Rs,i16
  case 0x06:
  //BGTZ Rs,i16
  case 0x07:
    return 1;

  }

  return 0;
}

#undef IpuReg
#undef VuReg
#undef R0
#undef Sa
#undef Rdn
#undef Rtn
#undef Rsn
#undef Vdn
#undef Vsn
#undef Vtn
#undef Rd
#undef Rt
#undef Rs
#undef Vd
#undef Vs
#undef Vt
#undef i16
#undef n16
#undef n26
#undef callvu
