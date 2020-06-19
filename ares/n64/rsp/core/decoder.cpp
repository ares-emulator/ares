#define OP pipeline.instruction
#define RD GPR[RDn]
#define RT GPR[RTn]
#define RS GPR[RSn]
#define VD VPR[VDn]
#define VS VPR[VSn]
#define VT VPR[VTn]

#define jp(id, name, ...) case id: return decoder##name(__VA_ARGS__)
#define op(id, name, ...) case id: return instruction##name(__VA_ARGS__)
#define br(id, name, ...) case id: return instruction##name(__VA_ARGS__)

auto RSP::decoderEXECUTE() -> void {
  #define DECODER_EXECUTE
  #include "decoder.hpp"
}

auto RSP::decoderSPECIAL() -> void {
  #define DECODER_SPECIAL
  #include "decoder.hpp"
}

auto RSP::decoderREGIMM() -> void {
  #define DECODER_REGIMM
  #include "decoder.hpp"
}

auto RSP::decoderCOP0() -> void {
  #define DECODER_COP0
  #include "decoder.hpp"
}

auto RSP::decoderCOP2() -> void {
  #define DECODER_COP2
  #include "decoder.hpp"
}

auto RSP::decoderLWC2() -> void {
  #define DECODER_LWC2
  #include "decoder.hpp"
}

auto RSP::decoderSWC2() -> void {
  #define DECODER_SWC2
  #include "decoder.hpp"
}

auto RSP::instructionINVALID() -> void {
}

#undef jp
#undef op
#undef br

#undef OP
#undef RD
#undef RT
#undef RS
#undef VD
#undef VS
#undef VT
