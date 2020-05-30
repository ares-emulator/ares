//{
  //scc-registers.cpp
  auto getControlRegister(uint) -> u32;
  auto setControlRegister(uint, uint32) -> void;

  //scc-instructions.cpp
  auto instructionMFC0(r32& rt, u8 rd) -> void;
  auto instructionMTC0(cr32& rt, u8 rd) -> void;
//};
