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
    auto ioSCC(bool mode, u32 address, u32 data) -> void;
    auto ioStatus(bool mode, u32 address, u32 data) -> void;

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

  auto power(bool reset) -> void;

  struct Pipeline {
    u32 address;
    u32 instruction;
  } pipeline;

  //dma.cpp
  auto dmaTransfer() -> void;

  //io.cpp
  auto readWord(u32 address) -> u32;
  auto writeWord(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct DMA {
    n1  pbusRegion;
    n12 pbusAddress;
    n24 dramAddress;

    struct Transfer {
      n12 length;
      n12 skip;
      n8  count;
    } read, write;

    struct Request {
      //serialization.cpp
      auto serialize(serializer&) -> void;

      enum class Type : u32 { Read, Write } type;
      n1  pbusRegion;
      n12 pbusAddress;
      n24 dramAddress;
      n16 length;
      n16 skip;
      n16 count;
    };
    nall::queue<Request[2]> requests;
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
  auto ADDIU(r32& rt, cr32& rs, s16 imm) -> void;
  auto ADDU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto AND(r32& rd, cr32& rs, cr32& rt) -> void;
  auto ANDI(r32& rt, cr32& rs, u16 imm) -> void;
  auto BEQ(cr32& rs, cr32& rt, s16 imm) -> void;
  auto BGEZ(cr32& rs, s16 imm) -> void;
  auto BGEZAL(cr32& rs, s16 imm) -> void;
  auto BGTZ(cr32& rs, s16 imm) -> void;
  auto BLEZ(cr32& rs, s16 imm) -> void;
  auto BLTZ(cr32& rs, s16 imm) -> void;
  auto BLTZAL(cr32& rs, s16 imm) -> void;
  auto BNE(cr32& rs, cr32& rt, s16 imm) -> void;
  auto BREAK() -> void;
  auto J(u32 imm) -> void;
  auto JAL(u32 imm) -> void;
  auto JALR(r32& rd, cr32& rs) -> void;
  auto JR(cr32& rs) -> void;
  auto LB(r32& rt, cr32& rs, s16 imm) -> void;
  auto LBU(r32& rt, cr32& rs, s16 imm) -> void;
  auto LH(r32& rt, cr32& rs, s16 imm) -> void;
  auto LHU(r32& rt, cr32& rs, s16 imm) -> void;
  auto LUI(r32& rt, u16 imm) -> void;
  auto LW(r32& rt, cr32& rs, s16 imm) -> void;
  auto NOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto OR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto ORI(r32& rt, cr32& rs, u16 imm) -> void;
  auto SB(cr32& rt, cr32& rs, s16 imm) -> void;
  auto SH(cr32& rt, cr32& rs, s16 imm) -> void;
  auto SLL(r32& rd, cr32& rt, u8 sa) -> void;
  auto SLLV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto SLT(r32& rd, cr32& rs, cr32& rt) -> void;
  auto SLTI(r32& rt, cr32& rs, s16 imm) -> void;
  auto SLTIU(r32& rt, cr32& rs, s16 imm) -> void;
  auto SLTU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto SRA(r32& rd, cr32& rt, u8 sa) -> void;
  auto SRAV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto SRL(r32& rd, cr32& rt, u8 sa) -> void;
  auto SRLV(r32& rd, cr32& rt, cr32& rs) -> void;
  auto SUBU(r32& rd, cr32& rs, cr32& rt) -> void;
  auto SW(cr32& rt, cr32& rs, s16 imm) -> void;
  auto XOR(r32& rd, cr32& rs, cr32& rt) -> void;
  auto XORI(r32& rt, cr32& rs, u16 imm) -> void;

  //scc.cpp: System Control Coprocessor
  auto MFC0(r32& rt, u8 rd) -> void;
  auto MTC0(cr32& rt, u8 rd) -> void;

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

  auto CFC2(r32& rt, u8 rd) -> void;
  auto CTC2(cr32& rt, u8 rd) -> void;
  auto LBV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LDV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LFV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LHV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LLV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LPV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LQV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LRV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LSV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LTV(u8 vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LUV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto LWV(r128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto MFC2(r32& rt, cr128& vs, u8 e) -> void;
  auto MTC2(cr32& rt, r128& vs, u8 e) -> void;
  auto SBV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SDV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SFV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SHV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SLV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SPV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SQV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SRV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SSV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto STV(u8 vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SUV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto SWV(cr128& vt, u8 e, cr32& rs, s8 imm) -> void;
  auto VABS(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VADD(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VADDC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VAND(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VCH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VCL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VCR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VEQ(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VGE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VLT(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool U>
  auto VMACF(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMACQ(r128& vd) -> void;
  auto VMADH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMADL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMADM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMADN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMOV(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto VMRG(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMUDH(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMUDL(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMUDM(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMUDN(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool U>
  auto VMULF(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VMULQ(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VNAND(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VNE(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VNOP() -> void;
  auto VNOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VNXOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VOR(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  template<bool L>
  auto VRCP(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto VRCPH(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  template<bool D>
  auto VRND(r128& vd, u8 vs, cr128& vt, u8 e) -> void;
  template<bool L>
  auto VRSQ(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto VRSQH(r128& vd, u8 de, cr128& vt, u8 e) -> void;
  auto VSAR(r128& vd, cr128& vs, u8 e) -> void;
  auto VSUB(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VSUBC(r128& vd, cr128& vs, cr128& vt, u8 e) -> void;
  auto VXOR(r128& rd, cr128& vs, cr128& vt, u8 e) -> void;

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

  auto INVALID() -> void;

  //recompiler.cpp
  struct Recompiler : recompiler::amd64 {
    using recompiler::amd64::call;
    RSP& self;
    Recompiler(RSP& self) : self(self) {}

    struct Block {
      auto execute(RSP& self) -> void {
        ((void (*)(r32*, RSP*, r128*))code)(&self.ipu.r[0], &self, &self.vpu.r[0]);
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

    template<typename R, typename... P> auto call(R (RSP::*function)(P...)) -> void;

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
