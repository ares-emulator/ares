//LSI CoreWare CW33300 (MIPS R3000A core)

struct CPU : Thread {
  Node::Object node;
  Memory::Writable ram;
  Memory::Writable scratchpad;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto exception(u8 code) -> void;
    auto interrupt(u8 mask) -> void;
    auto message() -> void;
    auto function() -> void;

    struct Memory {
      Node::Debugger::Memory ram;
      Node::Debugger::Memory scratchpad;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification exception;
      Node::Debugger::Tracer::Notification interrupt;
      Node::Debugger::Tracer::Notification message;
      Node::Debugger::Tracer::Notification function;
    } tracer;

  private:
    auto messageChar(char) -> void;
    auto messageText(u32) -> void;
  } debugger;

  using cs32 = const s32;
  using cu32 = const u32;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto synchronize() -> void;

  auto instruction() -> void;
  auto instructionEpilogue() -> bool;
  auto instructionHook() -> void;

  auto power(bool reset) -> void;

  struct Pipeline {
    u32 address = 0;
    u32 instruction = 0;
  } pipeline;

  //delay-slots.cpp
  struct Delay {
    //load delay slots
    struct {
      u32* target = nullptr;
      u32  source = 0;
    } load[2];

    //branch delay slots
    struct Branch {
      n1  slot;
      n1  take;
      n32 address;
    } branch[2];

    //interrupt delay counter
    u32 interrupt;
  } delay;

  auto load(u32& target) const -> u32;
  auto load(u32& target, u32 source) -> void;
  auto store(u32& target, u32 source) -> void;
  template<u32 N = 1> auto branch(u32 address, bool take = true) -> void;
  auto processDelayLoad() -> void;
  auto processDelayBranch() -> void;

  //memory.cpp
  auto fetch(u32 address) -> u32;

  template<u32 Size> auto read(u32 address) -> u32;
  template<u32 Size> auto write(u32 address, u32 data) -> void;

  //icache.cpp
  struct InstructionCache {
    auto step(u32 address) -> void;
    auto fetch(u32 address) -> u32;
    auto read(u32 address) -> u32;
    auto invalidate(u32 address) -> void;
    auto enable(bool) -> void;
    auto power(bool reset) -> void;

    //4KB
    struct Line {
      u32 words[4];
      u32 tag;
    } lines[256];
  } icache;

  //exceptions.cpp
  struct Exception {
    CPU& self;
    Exception(CPU& self) : self(self) {}

    auto operator()() -> bool;
    auto trigger(u32 code) -> void;

    auto interruptsPending() -> u8;
    auto interrupt() -> void;
    template<u32 Mode> auto address(u32 address) -> void;
    auto busInstruction() -> void;
    auto busData() -> void;
    auto systemCall() -> void;
    auto breakpoint(bool overrideVectorLocation) -> void;
    auto reservedInstruction() -> void;
    auto coprocessor() -> void;
    auto arithmeticOverflow() -> void;
    auto trap() -> void;

    bool triggered;
  } exception{*this};

  //breakpoints.cpp
  struct Breakpoint {
    CPU& self;
    Breakpoint(CPU& self) : self(self) {}

    auto testCode(u32 address) -> bool;
    template<u32 Mode, u32 Size> auto testData(u32 address) -> bool;

    u32 lastPC;
  } breakpoint{*this};

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct IPU {
    CPU& self;
    IPU(CPU& self) : self(self) {}

    u32 r[32];
    u32 lo;
    u32 hi;
    u32 pb;  //previous PC
    u32 pc;  //current PC
    u32 pd;  //next PC
  } ipu{*this};

  //interpreter-ipu.cpp
  auto ADD(u32& rd, cu32& rs, cu32& rt) -> void;
  auto ADDI(u32& rt, cu32& rs, s16 imm) -> void;
  auto ADDIU(u32& rt, cu32& rs, s16 imm) -> void;
  auto ADDU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto AND(u32& rd, cu32& rs, cu32& rt) -> void;
  auto ANDI(u32& rt, cu32& rs, u16 imm) -> void;
  auto BEQ(cu32& rs, cu32& rt, s16 imm) -> void;
  auto BGEZ(cs32& rs, s16 imm) -> void;
  auto BGEZAL(cs32& rs, s16 imm) -> void;
  auto BGTZ(cs32& rs, s16 imm) -> void;
  auto BLEZ(cs32& rs, s16 imm) -> void;
  auto BLTZ(cs32& rs, s16 imm) -> void;
  auto BLTZAL(cs32& rs, s16 imm) -> void;
  auto BNE(cu32& rs, cu32& rt, s16 imm) -> void;
  auto BREAK() -> void;
  auto DIV(cs32& rs, cs32& rt) -> void;
  auto DIVU(cu32& rs, cu32& rt) -> void;
  auto J(u32 imm) -> void;
  auto JAL(u32 imm) -> void;
  auto JALR(u32& rd, cu32& rs) -> void;
  auto JR(cu32& rs) -> void;
  auto LB(u32& rt, cu32& rs, s16 imm) -> void;
  auto LBU(u32& rt, cu32& rs, s16 imm) -> void;
  auto LH(u32& rt, cu32& rs, s16 imm) -> void;
  auto LHU(u32& rt, cu32& rs, s16 imm) -> void;
  auto LUI(u32& rt, u16 imm) -> void;
  auto LW(u32& rt, cu32& rs, s16 imm) -> void;
  auto LWL(u32& rt, cu32& rs, s16 imm) -> void;
  auto LWR(u32& rt, cu32& rs, s16 imm) -> void;
  auto MFHI(u32& rd) -> void;
  auto MFLO(u32& rd) -> void;
  auto MTHI(cu32& rs) -> void;
  auto MTLO(cu32& rs) -> void;
  auto MULT(cs32& rs, cs32& rt) -> void;
  auto MULTU(cu32& rs, cu32& rt) -> void;
  auto NOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto OR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto ORI(u32& rt, cu32& rs, u16 imm) -> void;
  auto SB(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SH(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SLL(u32& rd, cu32& rt, u8 sa) -> void;
  auto SLLV(u32& rd, cu32& rt, cu32& rs) -> void;
  auto SLT(u32& rd, cs32& rs, cs32& rt) -> void;
  auto SLTI(u32& rt, cs32& rs, s16 imm) -> void;
  auto SLTIU(u32& rt, cu32& rs, s16 imm) -> void;
  auto SLTU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto SRA(u32& rd, cs32& rt, u8 sa) -> void;
  auto SRAV(u32& rd, cs32& rt, cu32& rs) -> void;
  auto SRL(u32& rd, cu32& rt, u8 sa) -> void;
  auto SRLV(u32& rd, cu32& rt, cu32& rs) -> void;
  auto SUB(u32& rd, cu32& rs, cu32& rt) -> void;
  auto SUBU(u32& rd, cu32& rs, cu32& rt) -> void;
  auto SW(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SWL(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SWR(cu32& rt, cu32& rs, s16 imm) -> void;
  auto SYSCALL() -> void;
  auto XOR(u32& rd, cu32& rs, cu32& rt) -> void;
  auto XORI(u32& rt, cu32& rs, u16 imm) -> void;

  struct SCC {
    CPU& self;
    SCC(CPU& self) : self(self) {}

    struct Breakpoint {
      struct Address {
        // 3: Breakpoint Code Address
        n32 code;

        // 5: Breakpoint Data Address
        n32 data;
      } address;

      struct Mask {
        //11: Breakpoint Code Mask
        n32 code;

        // 9: Breakpoint Data Mask
        n32 data;
      } mask;

      // 7: Breakpoint Control
      struct Status {
        n1 any;
        n1 code;
        n1 data;
        n1 read;
        n1 write;
        n1 trace;
      } status;
      n2 redirection;
      n2 unknown;
      struct Test {
        n1 code;
        n1 data;
        n1 read;
        n1 write;
        n1 trace;
      } test;
      struct Enable {
        n1 master;
        n1 kernel;
        n1 user;
        n1 trap;
      } enable;
    } breakpoint;

    // 6: Target Address
    n32 targetAddress;

    // 8: Bad Virtual Address
    n32 badVirtualAddress;

    //12: Status
    struct Status {
      struct Frame {
        n1 interruptEnable;
        n1 userMode;
      } frame[3];
      n8 interruptMask;
      struct Cache {
        n1 isolate;
        n1 swap;
        n1 parityZero;
        n1 loadWasData;
        n1 parityError;
      } cache;
      n1 tlbShutdown;
      n1 vectorLocation;
      n1 reverseEndian;
      struct Enable {
        n1 coprocessor0 = 1;
        n1 coprocessor1;
        n1 coprocessor2 = 1;
        n1 coprocessor3;
      } enable;
    } status;

    //13: Cause
    struct Cause {
      n5 exceptionCode;
      n8 interruptPending;
      n2 coprocessorError;
      n1 branchTaken;
      n1 branchDelay;
    } cause;

    //14: Exception Program Counter
    n32 epc;

    //15: Product ID
    struct ProductID {
      static constexpr u8 implementation = 0x00;
      static constexpr u8 revision = 0x02;
    } productID;
  } scc{*this};

  //interpreter-scc.cpp
  auto getControlRegisterSCC(u8) -> u32;
  auto setControlRegisterSCC(u8, u32) -> void;

  auto MFC0(u32& rt, u8 rd) -> void;
  auto MTC0(cu32& rt, u8 rd) -> void;
  auto RFE() -> void;

  struct GTE {
    CPU& self;
    GTE(CPU& self) : self(self) {}

    //color
    struct c32 {
      u8 r, g, b, t;
    };

    //screen point
    struct p16 {
      s16 x, y;
      u16 z;
    };

    //16-bit vector
    struct v16 { union {
      struct { s16 x, y, z; };
      struct { s16 r, g, b; };
    };};

    //32-bit vector
    struct v32 { union {
      struct { s32 x, y, z; };
      struct { s32 r, g, b; };
    };};

    //64-bit vector
    struct v64 {
      s64 x, y, z;
    };

    //16-bit vector with temporary
    struct v16t : v16 {
      s16 t;
    };

    //32-bit vector with temporary
    struct v32t : v32 {
      s32 t;
    };

    //16-bit matrix
    struct m16 {
      v16 a, b, c;
    };

    //interpreter-gte.cpp
    auto constructTable() -> void;

    auto countLeadingZeroes16(u16) -> u32;
    auto countLeadingZeroes32(u32) -> u32;

    auto getDataRegister(u32) -> u32;
    auto setDataRegister(u32, u32) -> void;

    auto getControlRegister(u32) -> u32;
    auto setControlRegister(u32, u32) -> void;

    template<u32> auto checkMac(s64 value) -> s64;
    template<u32> auto extend(s64 mac) -> s64;
    template<u32> auto saturateIr(s32 value, bool lm = 0) -> s32;
    template<u32> auto saturateColor(s32 value) -> u8;

    template<u32> auto setMac(s64 value) -> s64;
    template<u32> auto setIr(s32 value, bool lm = 0) -> void;
    template<u32> auto setMacAndIr(s64 value, bool lm = 0) -> void;
    auto setMacAndIr(const v64& vector) -> void;
    auto setOtz(s64 value) -> void;

    auto matrixMultiply(const m16&, const v16&, const v32& = {0, 0, 0}) -> v64;
    auto vectorMultiply(const v16&, const v16&, const v16& = {0, 0, 0}) -> v64;
    auto vectorMultiply(const v16&, s16) -> v64;
    auto divide(u32 lhs, u32 rhs) -> u32;
    auto pushScreenX(s32 sx) -> void;
    auto pushScreenY(s32 sy) -> void;
    auto pushScreenZ(s32 sz) -> void;
    auto pushColor(s32 r, s32 g, s32 b) -> void;
    auto pushColor() -> void;

    auto prologue() -> void;
    auto prologue(bool lm, u8 sf) -> void;
    auto epilogue() -> void;

    m16  v;                //VX, VY, VZ
    c32  rgbc;
    u16  otz;
    v16t ir;
    p16  screen[4];        //SX,  SY,  SZ (screen[3].{x,y} do not exist)
    u32  rgb[4];           //RGB3 is reserved
    v32t mac;
    u32  lzcs, lzcr;
    m16  rotation;         //RT1, RT2, RT3
    v32  translation;      //TRX, TRY, TRZ
    m16  light;            //L1,  L2,  L3
    v32  backgroundColor;  //RBK, GBK, BBK
    m16  color;            //LR,  LG,  LB
    v32  farColor;         //RFC, GFC, BFC
    s32  ofx, ofy;
    u16  h;
    s16  dqa;
    s32  dqb;
    s16  zsf3, zsf4;
    struct Flag {
      u32 value;
      BitField<32, 12> ir0_saturated  {&value};
      BitField<32, 13> sy2_saturated  {&value};
      BitField<32, 14> sx2_saturated  {&value};
      BitField<32, 15> mac0_underflow {&value};
      BitField<32, 16> mac0_overflow  {&value};
      BitField<32, 17> divide_overflow{&value};
      BitField<32, 18> sz3_saturated  {&value};
      BitField<32, 18> otz_saturated  {&value};
      BitField<32, 19> b_saturated    {&value};
      BitField<32, 20> g_saturated    {&value};
      BitField<32, 21> r_saturated    {&value};
      BitField<32, 22> ir3_saturated  {&value};
      BitField<32, 23> ir2_saturated  {&value};
      BitField<32, 24> ir1_saturated  {&value};
      BitField<32, 25> mac3_underflow {&value};
      BitField<32, 26> mac2_underflow {&value};
      BitField<32, 27> mac1_underflow {&value};
      BitField<32, 28> mac3_overflow  {&value};
      BitField<32, 29> mac2_overflow  {&value};
      BitField<32, 30> mac1_overflow  {&value};
      BitField<32, 31> error          {&value};
    } flag;
    bool lm;
    u8   tv;
    u8   mv;
    u8   mm;
    u8   sf;

  //unserialized:
    u8 unsignedNewtonRaphsonTable[257];
  } gte{*this};

  //interpreter-gte.cpp
  auto AVSZ3() -> void;
  auto AVSZ4() -> void;
  auto CC(bool lm, u8 sf) -> void;
  auto CDP(bool lm, u8 sf) -> void;
  auto CFC2(u32& rt, u8 rd) -> void;
  auto CTC2(cu32& rt, u8 rd) -> void;
  auto DCPL(bool lm, u8 sf) -> void;
  auto DPC(const GTE::v16&) -> void;
  auto DPCS(bool lm, u8 sf) -> void;
  auto DPCT(bool lm, u8 sf) -> void;
  auto GPF(bool lm, u8 sf) -> void;
  auto GPL(bool lm, u8 sf) -> void;
  auto INTPL(bool lm, u8 sf) -> void;
  auto LWC2(u8 rt, cu32& rs, s16 imm) -> void;
  auto MFC2(u32& rt, u8 rd) -> void;
  auto MTC2(cu32& rt, u8 rd) -> void;
  auto MVMVA(bool lm, u8 tv, u8 mv, u8 mm, u8 sf) -> void;
  template<u32> auto NC(const GTE::v16&) -> void;
  auto NCCS(bool lm, u8 sf) -> void;
  auto NCCT(bool lm, u8 sf) -> void;
  auto NCDS(bool lm, u8 sf) -> void;
  auto NCDT(bool lm, u8 sf) -> void;
  auto NCLIP() -> void;
  auto NCS(bool lm, u8 sf) -> void;
  auto NCT(bool lm, u8 sf) -> void;
  auto OP(bool lm, u8 sf) -> void;
  auto RTP(GTE::v16, bool last) -> void;
  auto RTPS(bool lm, u8 sf) -> void;
  auto RTPT(bool lm, u8 sf) -> void;
  auto SQR(bool lm, u8 sf) -> void;
  auto SWC2(u8 rt, cu32& rs, s16 imm) -> void;

  //decoder.cpp
  auto decoderEXECUTE() -> void;
  auto decoderSPECIAL() -> void;
  auto decoderREGIMM() -> void;
  auto decoderSCC() -> void;
  auto decoderGTE() -> void;

  auto COP1() -> void;
  auto COP3() -> void;
  auto LWC0(u8 rt, cu32& rs, s16 imm) -> void;
  auto LWC1(u8 rt, cu32& rs, s16 imm) -> void;
  auto LWC3(u8 rt, cu32& rs, s16 imm) -> void;
  auto SWC0(u8 rt, cu32& rs, s16 imm) -> void;
  auto SWC1(u8 rt, cu32& rs, s16 imm) -> void;
  auto SWC3(u8 rt, cu32& rs, s16 imm) -> void;
  auto INVALID() -> void;

  //recompiler.cpp
  struct Recompiler : recompiler::amd64 {
    using recompiler::amd64::call;
    CPU& self;
    Recompiler(CPU& self) : self(self) {}

    struct Block {
      auto execute(CPU& self) -> void {
        ((void (*)(u32*, CPU*))code)(&self.ipu.r[0], &self);
      }

      u8* code;
    };

    struct Pool {
      Block* blocks[1 << 6];
    };

    auto reset() -> void {
      for(u32 index : range(1 << 21)) pools[index] = nullptr;
    }

    auto invalidate(u32 address) -> void {
      pools[address >> 8 & 0x1fffff] = nullptr;
    }

    auto pool(u32 address) -> Pool*;
    auto block(u32 address) -> Block*;

    auto emit(u32 address) -> Block*;
    auto emitEXECUTE(u32 instruction) -> bool;
    auto emitSPECIAL(u32 instruction) -> bool;
    auto emitREGIMM(u32 instruction) -> bool;
    auto emitSCC(u32 instruction) -> bool;
    auto emitGTE(u32 instruction) -> bool;

    template<typename R, typename... P> auto call(R (CPU::*function)(P...)) -> void;

    bump_allocator allocator;
    Pool* pools[1 << 21];  //2_MiB * sizeof(void*) = 16_MiB
  } recompiler{*this};

  struct Disassembler {
    CPU& self;
    Disassembler(CPU& self) : self(self) {}

    //disassembler.cpp
    auto disassemble(u32 address, u32 instruction) -> string;
    template<typename... P> auto hint(P&&... p) const -> string;

    bool showColors = true;
    bool showValues = true;

  //private:
    auto EXECUTE() -> vector<string>;
    auto SPECIAL() -> vector<string>;
    auto REGIMM() -> vector<string>;
    auto SCC() -> vector<string>;
    auto GTE() -> vector<string>;
    auto immediate(s64 value, u8 bits = 0) const -> string;
    auto ipuRegisterName(u8 index) const -> string;
    auto ipuRegisterValue(u8 index) const -> string;
    auto ipuRegisterIndex(u8 index, s16 offset) const -> string;
    auto sccRegisterName(u8 index) const -> string;
    auto sccRegisterValue(u8 index) const -> string;
    auto gteDataRegisterName(u8 index) const -> string;
    auto gteDataRegisterValue(u8 index) const -> string;
    auto gteControlRegisterName(u8 index) const -> string;
    auto gteControlRegisterValue(u8 index) const -> string;

    u32 address;
    u32 instruction;
  } disassembler{*this};
};

extern CPU cpu;
