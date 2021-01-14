#define OP pipeline.instruction
#define RD ipu.r[RDn]
#define RT ipu.r[RTn]
#define RS ipu.r[RSn]

#define jp(id, name, ...) case id: return decoder##name(__VA_ARGS__)
#define op(id, name, ...) case id: return instruction##name(__VA_ARGS__)
#define br(id, name, ...) case id: return instruction##name(__VA_ARGS__)

auto CPU::decoderEXECUTE() -> void {
  #define DECODER_EXECUTE
  #include "decoder.hpp"
}

auto CPU::decoderSPECIAL() -> void {
  #define DECODER_SPECIAL
  #include "decoder.hpp"
}

auto CPU::decoderREGIMM() -> void {
  #define DECODER_REGIMM
  #include "decoder.hpp"
}

auto CPU::decoderSCC() -> void {
  if(!scc.status.enable.coprocessor0) {
    if(context.mode != Context::Mode::Kernel) return exception.coprocessor0();
  }

  #define DECODER_SCC
  #include "decoder.hpp"

  //undefined instructions do not throw a reserved instruction exception
}

auto CPU::decoderFPU() -> void {
  if(!scc.status.enable.coprocessor1) {
    return exception.coprocessor1();
  }

  #define DECODER_FPU
  #include "decoder.hpp"

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
#undef RD
#undef RT
#undef RS
