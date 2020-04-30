//NEC VR4300
//{
  auto instructionCOP0() -> void {}
  auto instructionCOP1() -> void {}
  auto instructionCOP2() -> void {}
  auto instructionCOP3() -> void {}

  //core.cpp
  auto powerR4300() -> void;

  //memory.cpp
  auto readByte(uint32_t address) -> maybe<uint32_t>;
  auto readHalf(uint32_t address) -> maybe<uint32_t>;
  auto readWord(uint32_t address) -> maybe<uint32_t>;
  auto readWord(uint32_t address, uint32_t mask) -> maybe<uint32_t>;
  auto readDouble(uint32_t address) -> maybe<uint64_t>;
  auto readDouble(uint32_t address, uint32_t mask) -> maybe<uint64_t>;
  auto writeByte(uint32_t address, uint8_t data) -> void;
  auto writeHalf(uint32_t address, uint16_t data) -> void;
  auto writeWord(uint32_t address, uint32_t data) -> void;
  auto writeWord(uint32_t address, uint32_t mask, uint32_t data) -> void;
  auto writeDouble(uint32_t address, uint64_t data) -> void;
  auto writeDouble(uint32_t address, uint32_t mask, uint64_t data) -> void;

  //instruction.cpp
  auto exception(uint type) -> void;
  auto instruction() -> void;
  auto instructionSpecial() -> void;
  auto instructionRegisterImmediate() -> void;

  //instructions.cpp
  auto instructionADD() -> void;
  auto instructionADDI() -> void;
  auto instructionADDIU() -> void;
  auto instructionADDU() -> void;
  auto instructionAND() -> void;
  auto instructionANDI() -> void;
  auto instructionBEQ() -> void;
  auto instructionBEQL() -> void;
  auto instructionBGEZ() -> void;
  auto instructionBGEZAL() -> void;
  auto instructionBGEZALL() -> void;
  auto instructionBGEZL() -> void;
  auto instructionBGTZ() -> void;
  auto instructionBGTZL() -> void;
  auto instructionBLEZ() -> void;
  auto instructionBLEZL() -> void;
  auto instructionBLTZ() -> void;
  auto instructionBLTZAL() -> void;
  auto instructionBLTZALL() -> void;
  auto instructionBLTZL() -> void;
  auto instructionBNE() -> void;
  auto instructionBNEL() -> void;
  auto instructionBREAK() -> void;
  auto instructionCACHE() -> void;
  auto instructionDADD() -> void;
  auto instructionDADDI() -> void;
  auto instructionDADDIU() -> void;
  auto instructionDADDU() -> void;
  auto instructionDDIV() -> void;
  auto instructionDDIVU() -> void;
  auto instructionDIV() -> void;
  auto instructionDIVU() -> void;
  auto instructionDMULT() -> void;
  auto instructionDMULTU() -> void;
  auto instructionDSLL() -> void;
  auto instructionDSLL32() -> void;
  auto instructionDSLLV() -> void;
  auto instructionDSRA() -> void;
  auto instructionDSRA32() -> void;
  auto instructionDSRAV() -> void;
  auto instructionDSRL() -> void;
  auto instructionDSRL32() -> void;
  auto instructionDSRLV() -> void;
  auto instructionDSUB() -> void;
  auto instructionDSUBU() -> void;
  auto instructionJ() -> void;
  auto instructionJAL() -> void;
  auto instructionJALR() -> void;
  auto instructionJR() -> void;
  auto instructionLB() -> void;
  auto instructionLBU() -> void;
  auto instructionLD() -> void;
  auto instructionLDC1() -> void;
  auto instructionLDC2() -> void;
  auto instructionLDL() -> void;
  auto instructionLDR() -> void;
  auto instructionLH() -> void;
  auto instructionLHU() -> void;
  auto instructionLUI() -> void;
  auto instructionLL() -> void;
  auto instructionLLD() -> void;
  auto instructionLW() -> void;
  auto instructionLWC1() -> void;
  auto instructionLWC2() -> void;
  auto instructionLWC3() -> void;
  auto instructionLWL() -> void;
  auto instructionLWR() -> void;
  auto instructionLWU() -> void;
  auto instructionMFHI() -> void;
  auto instructionMFLO() -> void;
  auto instructionMTHI() -> void;
  auto instructionMTLO() -> void;
  auto instructionMULT() -> void;
  auto instructionMULTU() -> void;
  auto instructionNOR() -> void;
  auto instructionOR() -> void;
  auto instructionORI() -> void;
  auto instructionSB() -> void;
  auto instructionSC() -> void;
  auto instructionSD() -> void;
  auto instructionSCD() -> void;
  auto instructionSDC1() -> void;
  auto instructionSDC2() -> void;
  auto instructionSDL() -> void;
  auto instructionSDR() -> void;
  auto instructionSH() -> void;
  auto instructionSLL() -> void;
  auto instructionSLLV() -> void;
  auto instructionSLT() -> void;
  auto instructionSLTI() -> void;
  auto instructionSLTIU() -> void;
  auto instructionSLTU() -> void;
  auto instructionSRA() -> void;
  auto instructionSRAV() -> void;
  auto instructionSRL() -> void;
  auto instructionSRLV() -> void;
  auto instructionSUB() -> void;
  auto instructionSUBU() -> void;
  auto instructionSW() -> void;
  auto instructionSWC1() -> void;
  auto instructionSWC2() -> void;
  auto instructionSWC3() -> void;
  auto instructionSWL() -> void;
  auto instructionSWR() -> void;
  auto instructionSYNC() -> void;
  auto instructionSYSCALL() -> void;
  auto instructionTEQ() -> void;
  auto instructionTEQI() -> void;
  auto instructionTGE() -> void;
  auto instructionTGEI() -> void;
  auto instructionTGEIU() -> void;
  auto instructionTGEU() -> void;
  auto instructionTLT() -> void;
  auto instructionTLTI() -> void;
  auto instructionTLTIU() -> void;
  auto instructionTLTU() -> void;
  auto instructionTNE() -> void;
  auto instructionTNEI() -> void;
  auto instructionXOR() -> void;
  auto instructionXORI() -> void;

  //serialization.cpp
  auto serializeR4300(serializer&) -> void;

  //disassembler.cpp
  auto disassembleInstruction() -> string;
  auto disassembleInstructionDecode() -> vector<string>;
  auto disassembleInstructionSpecial() -> vector<string>;
  auto disassembleInstructionRegisterImmediate() -> vector<string>;
  auto disassembleContext() -> string;

  //exceptions
  enum : uint {
    Break,
    InvalidInstruction,
    Overflow,
    Syscall,
    Trap,
  };

  struct Context {
    uint64_t gpr[32];
    uint64_t lo;
    uint64_t hi;
    uint64_t pc;
        bool llbit;

    //internal
    uint64_t ip;
  } r;

  uint32_t opcode;

  static constexpr bool Endian = 1;  //0 = little, 1 = big
  static constexpr uint FlipLE = (Endian == 0 ? 7 : 0);
  static constexpr uint FlipBE = (Endian == 1 ? 7 : 0);
//};
