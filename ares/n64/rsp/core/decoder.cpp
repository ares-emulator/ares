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
  #undef DECODER_EXECUTE
}

auto RSP::decoderSPECIAL() -> void {
  #define DECODER_SPECIAL
  #include "decoder.hpp"
  #undef DECODER_SPECIAL
}

auto RSP::decoderREGIMM() -> void {
  #define DECODER_REGIMM
  #include "decoder.hpp"
  #undef DECODER_REGIMM
}

auto RSP::decoderCOP0() -> void {
  #define DECODER_COP0
  #include "decoder.hpp"
  #undef DECODER_COP0
}

auto RSP::decoderCOP2() -> void {
  #define DECODER_COP2
  #include "decoder.hpp"
  #undef DECODER_COP2
}

auto RSP::decoderLWC2() -> void {
  #define DECODER_LWC2
  #include "decoder.hpp"
  #undef DECODER_LWC2
}

auto RSP::decoderSWC2() -> void {
  #define DECODER_SWC2
  #include "decoder.hpp"
  #undef DECODER_SWC2
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
