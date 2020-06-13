#define OP pipeline.instruction
#define RD GPR[RDn]
#define RT GPR[RTn]
#define RS GPR[RSn]

#define jp(id, name, ...) case id: return decoder##name(__VA_ARGS__)
#define op(id, name, ...) case id: return instruction##name(__VA_ARGS__)
#define br(id, name, ...) case id: return instruction##name(__VA_ARGS__)

auto CPU::decoderEXECUTE() -> void {
  #define DECODER_EXECUTE
  #include "decoder.hpp"
  #undef DECODER_EXECUTE
}

auto CPU::decoderSPECIAL() -> void {
  #define DECODER_SPECIAL
  #include "decoder.hpp"
  #undef DECODER_SPECIAL
}

auto CPU::decoderREGIMM() -> void {
  #define DECODER_REGIMM
  #include "decoder.hpp"
  #undef DECODER_REGIMM
}

auto CPU::decoderCOP0() -> void {
  if(!scc.status.enable.coprocessor0) {
    if(context.mode != Context::Mode::Kernel) return exception.coprocessor0();
  }

  #define DECODER_COP0
  #include "decoder.hpp"
  #undef DECODER_COP0

  //undefined instructions do not throw a reserved instruction exception
}

auto CPU::decoderCOP1() -> void {
  if(!scc.status.enable.coprocessor1) {
    return exception.coprocessor1();
  }

  #define DECODER_COP1
  #include "decoder.hpp"
  #undef DECODER_COP1

  //undefined instructions do not throw a reserved instruction exception
}

auto CPU::instructionCOP2() -> void {
  exception.coprocessor2();
}

auto CPU::instructionCOP3() -> void {
  exception.coprocessor3();
}

auto CPU::instructionINVALID() -> void {
  exception.reservedInstruction();
}

#undef jp
#undef op
#undef br

#undef OP
#undef SA
#undef RD
#undef RT
#undef RS
