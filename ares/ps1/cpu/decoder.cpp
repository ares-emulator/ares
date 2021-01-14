#define OPCODE pipeline.instruction
#define RD ipu.r[RDn]
#define RT ipu.r[RTn]
#define RS ipu.r[RSn]

#define jp(   id, name, ...) case id: return decoder##name(__VA_ARGS__)
#define op(   id, name, ...) case id: return instruction##name(__VA_ARGS__)
#define brIPU(id, name, ...) case id: return ipu.name(__VA_ARGS__)
#define opIPU(id, name, ...) case id: return ipu.name(__VA_ARGS__)
#define opSCC(id, name, ...) case id: return scc.name(__VA_ARGS__)
#define opGTE(id, name, ...) case id: return gte.name(__VA_ARGS__)

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
  #define DECODER_SCC
  #include "decoder.hpp"
}

auto CPU::decoderGTE() -> void {
  #define DECODER_GTE
  #include "decoder.hpp"
}

auto CPU::instructionCOP1() -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor();
}

auto CPU::instructionCOP3() -> void {
  if(!scc.status.enable.coprocessor3) return exception.coprocessor();
}

auto CPU::instructionLWC0(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor0) return exception.coprocessor();
  read<Word>(rs + imm);  //write target unknown
}

auto CPU::instructionLWC1(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor();
  read<Word>(rs + imm);  //write target unknown
}

auto CPU::instructionLWC3(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor3) return exception.coprocessor();
  read<Word>(rs + imm);  //write target unknown
}

auto CPU::instructionSWC0(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor0) return exception.coprocessor();
  write<Word>(rs + imm, 0);  //read source unknown
}

auto CPU::instructionSWC1(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor1) return exception.coprocessor();
  write<Word>(rs + imm, 0);  //read source unknown
}

auto CPU::instructionSWC3(u8 rt, cu32& rs, s16 imm) -> void {
  if(!scc.status.enable.coprocessor3) return exception.coprocessor();
  write<Word>(rs + imm, 0);  //read source unknown
}

auto CPU::instructionINVALID() -> void {
  return exception.reservedInstruction();
}

#undef jp
#undef op
#undef brIPU
#undef opIPU
#undef opSCC
#undef opGTE

#undef OPCODE
#undef RD
#undef RT
#undef RS
