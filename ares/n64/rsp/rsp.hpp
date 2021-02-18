//Reality Signal Processor

struct RSP : Thread, Memory::IO<RSP> {
  Node::Object node;
  Memory::Writable dmem;
  Memory::Writable imem;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto instruction() -> void;
    auto io(string_view) -> void;

    struct Memory {
      Node::Debugger::Memory dmem;
      Node::Debugger::Memory imem;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //rsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionDebug() -> void;

  auto power(bool reset) -> void;

  struct Pipeline {
    u32 address;
    u32 instruction;
  } pipeline;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct DMA {
    n1  memSource;
    n12 memAddress;
    n24 dramAddress;
    n1  busy;
    n1  full;

    struct Transfer {
      n12 length;
      n12 skip;
      n8  count;
    } read, write;
  } dma;

  struct Status : Memory::IO<Status> {
    RSP& self;
    Status(RSP& self) : self(self) {}

    //io.cpp
    auto readWord(u32 address) -> u32;
    auto writeWord(u32 address, u32 data) -> void;

    n1 semaphore;
    n1 halted = 1;
    n1 broken;
    n1 full;
    n1 singleStep;
    n1 interruptOnBreak;
    n1 signal[8];
  } status{*this};

  //ipu.cpp
  union r32 {
    struct {  int32_t s32; };
    struct { uint32_t u32; };
  };
  using cr32 = const r32;

  struct IPU {
    enum Register : u32 {
      R0, AT, V0, V1, A0, A1, A2, A3,
      T0, T1, T2, T3, T4, T5, T6, T7,
      S0, S1, S2, S3, S4, S5, S6, S7,
      T8, T9, K0, K1, GP, SP, S8, RA,
    };

    r32 r[32];
    u32 pc;
  } ipu;

  struct Branch {
    enum : u32 { Step, Take, DelaySlot, Halt };

    auto inDelaySlot() const -> bool { return state == DelaySlot; }
    auto reset() -> void { state = Step; }
    auto take(u32 address) -> void { state = Take; pc = address; }
    auto delaySlot() -> void { state = DelaySlot; }
    auto halt() -> void { state = Halt; }

    u64 pc = 0;
    u32 state = Step;
  } branch;

  //cpu-instructions.cpp
  auto instructionADDIU(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionADDU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionAND(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionANDI(r32& rt, cr32& rs, u16 imm) -> void;
  auto instructionBEQ(cr32& rs, cr32& rt, s16 imm) -> void;
  auto instructionBGEZ(cr32& rs, s16 imm) -> void;
  auto instructionBGEZAL(cr32& rs, s16 imm) -> void;
  auto instructionBGTZ(cr32& rs, s16 imm) -> void;
  auto instructionBLEZ(cr32& rs, s16 imm) -> void;
  auto instructionBLTZ(cr32& rs, s16 imm) -> void;
  auto instructionBLTZAL(cr32& rs, s16 imm) -> void;
  auto instructionBNE(cr32& rs, cr32& rt, s16 imm) -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE(u8 cache, u8 operation) -> void;
  auto instructionJ(u32 imm) -> void;
  auto instructionJAL(u32 imm) -> void;
  auto instructionJALR(r32& rd, cr32& rs) -> void;
  auto instructionJR(cr32& rs) -> void;
  auto instructionLB(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionLBU(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionLH(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionLHU(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionLUI(r32& rt, u16 imm) -> void;
  auto instructionLW(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionNOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionORI(r32& rt, cr32& rs, u16 imm) -> void;
  auto instructionSB(cr32& rt, cr32& rs, s16 imm) -> void;
  auto instructionSH(cr32& rt, cr32& rs, s16 imm) -> void;
  auto instructionSLL(r32& rd, cr32& rt, u8 sa) -> void;
  auto instructionSLLV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto instructionSLT(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionSLTI(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionSLTIU(r32& rt, cr32& rs, s16 imm) -> void;
  auto instructionSLTU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionSRA(r32& rd, cr32& rt, u8 sa) -> void;
  auto instructionSRAV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto instructionSRL(r32& rd, cr32& rt, u8 sa) -> void;
  auto instructionSRLV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto instructionSUBU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionSW(cr32& rt, cr32& rs, s16 imm) -> void;
  auto instructionXOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto instructionXORI(r32& rt, cr32& rs, u16 imm) -> void;

  //scc.cpp: System Control Coprocessor
  auto instructionMFC0(r32& rt, u8 rd) -> void;
  auto instructionMTC0(cr32& rt, u8 rd) -> void;

  //vpu.cpp: Vector Processing Unit
  union r128 {
    struct { uint128_t u128; };
    struct {   __m128i v128; };

    operator __m128i() const { return v128; }
    auto operator=(__m128i value) { v128 = value; }

    auto byte(u32 index) -> uint8_t& { return ((uint8_t*)&u128)[15 - index]; }
    auto byte(u32 index) const -> uint8_t { return ((uint8_t*)&u128)[15 - index]; }

    auto element(u32 index) -> uint16_t& { return ((uint16_t*)&u128)[7 - index]; }
    auto element(u32 index) const -> uint16_t { return ((uint16_t*)&u128)[7 - index]; }

    auto u8(u32 index) -> uint8_t& { return ((uint8_t*)&u128)[15 - index]; }
    auto u8(u32 index) const -> uint8_t { return ((uint8_t*)&u128)[15 - index]; }

    auto s16(u32 index) -> int16_t& { return ((int16_t*)&u128)[7 - index]; }
    auto s16(u32 index) const -> int16_t { return ((int16_t*)&u128)[7 - index]; }

    auto u16(u32 index) -> uint16_t& { return ((uint16_t*)&u128)[7 - index]; }
    auto u16(u32 index) const -> uint16_t { return ((uint16_t*)&u128)[7 - index]; }

    //VCx registers
    auto get(u32 index) const -> bool { return u16(index) != 0; }
    auto set(u32 index, bool value) -> bool { return u16(index) = 0 - value, value; }

    //vu-registers.cpp
    auto operator()(u32 index) const -> r128;
  };
  using cr128 = const r128;

  struct VU {
    r128 r[32];
    r128 acch, accm, accl;
    r128 vcoh, vcol;  //16-bit little endian
    r128 vcch, vccl;  //16-bit little endian
    r128 vce;         // 8-bit little endian
     s16 divin;
     s16 divout;
    bool divdp;
  } vpu;

  static constexpr r128 zero{0};
  static constexpr r128 invert{u128(0) - 1};

  auto accumulatorGet(u32 index) const -> u64;
  auto accumulatorSet(u32 index, u64 value) -> void;
  auto accumulatorSaturate(u32 index, bool slice, u16 negative, u16 positive) const -> u16;

  auto instructionCFC2(r32& rt, u8 rd) -> void;
  auto instructionCTC2(cr32& rt, u8 rd) -> void;
  auto instructionLBV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLDV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLFV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLHV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLLV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLPV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLQV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLRV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLSV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLTV(u8 vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLUV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionLWV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionMFC2(r32& rt, cr128& vs, u8 e) -> void;
  auto instructionMTC2(cr32& rt, r128& vs, u8 e) -> void;
  auto instructionSBV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSDV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSFV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSHV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSLV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSPV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSQV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSRV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSSV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSTV(u8 vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSUV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionSWV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto instructionVABS(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVADD(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVADDC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVAND(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVCH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVCL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVCR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVEQ(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVGE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVLT(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool U>
  auto instructionVMACF(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMACQ(r128& vd) -> void;
  auto instructionVMADH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMADL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMADM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMADN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMOV(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVMRG(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMUDN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool U>
  auto instructionVMULF(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVMULQ(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNAND(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNOP() -> void;
  auto instructionVNOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVNXOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool L>
  auto instructionVRCP(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVRCPH(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  template<bool D>
  auto instructionVRND(r128& vd, u8 vs, cr128& vt, u8 e) -> void;
  template<bool L>
  auto instructionVRSQ(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVRSQH(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto instructionVSAR(r128& vd, cr128& vs, u8 e) -> void;
  auto instructionVSUB(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVSUBC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto instructionVXOR(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;

//unserialized:
  u16 reciprocals[512];
  u16 inverseSquareRoots[512];

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderSCC() -> void;
  auto decoderVU() -> void;
  auto decoderLWC2() -> void;
  auto decoderSWC2() -> void;

  auto instructionINVALID() -> void;

  //recompiler.cpp
  struct Recompiler : recompiler::amd64 {
    RSP& self;
    Recompiler(RSP& self) : self(self) {}

    struct Block {
      auto execute() -> void {
        ((void (*)())code)();
      }

      u8* code;
    };

    struct Pool {
      auto operator==(const Pool& source) const -> bool { return hashcode == source.hashcode; }
      auto operator< (const Pool& source) const -> bool { return hashcode <  source.hashcode; }
      auto hash() const -> u32 { return hashcode; }

      u32 hashcode;
      Block* blocks[1024];
    };

    auto reset() -> void {
      context = nullptr;
      pools.reset();
    }

    auto invalidate() -> void {
      context = nullptr;
    }

    auto pool() -> Pool*;
    auto block(u32 address) -> Block*;

    auto emit(u32 address) -> Block*;
    auto emitEXECUTE(u32 instruction) -> bool;
    auto emitSPECIAL(u32 instruction) -> bool;
    auto emitREGIMM(u32 instruction) -> bool;
    auto emitSCC(u32 instruction) -> bool;
    auto emitVU(u32 instruction) -> bool;
    auto emitLWC2(u32 instruction) -> bool;
    auto emitSWC2(u32 instruction) -> bool;

    bump_allocator allocator;
    Pool* context = nullptr;
    set<Pool> pools;
  //hashset<Pool> pools;
  } recompiler{*this};

  struct Disassembler {
    RSP& self;
    Disassembler(RSP& self) : self(self) {}

    //disassembler.cpp
    auto disassemble(u32 address, u32 instruction) -> string;
    template<typename... P> auto hint(P&&... p) const -> string;

    bool showColors = true;
    bool showValues = true;

  private:
    auto EXECUTE() -> vector<string>;
    auto SPECIAL() -> vector<string>;
    auto REGIMM() -> vector<string>;
    auto SCC() -> vector<string>;
    auto LWC2() -> vector<string>;
    auto SWC2() -> vector<string>;
    auto VU() -> vector<string>;
    auto immediate(s64 value, u32 bits = 0) const -> string;
    auto ipuRegisterName(u32 index) const -> string;
    auto ipuRegisterValue(u32 index) const -> string;
    auto ipuRegisterIndex(u32 index, s16 offset) const -> string;
    auto sccRegisterName(u32 index) const -> string;
    auto sccRegisterValue(u32 index) const -> string;
    auto vpuRegisterName(u32 index, u32 element = 0) const -> string;
    auto vpuRegisterValue(u32 index, u32 element = 0) const -> string;
    auto ccrRegisterName(u32 index) const -> string;
    auto ccrRegisterValue(u32 index) const -> string;

    u32 address;
    u32 instruction;
  } disassembler{*this};
};

extern RSP rsp;
