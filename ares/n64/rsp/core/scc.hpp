//{
  //scc-registers.cpp
  auto getControlRegister(uint) -> u32;
  auto setControlRegister(uint, uint32) -> void;

  //scc-instructions.cpp
  auto instructionMFC0() -> void;
  auto instructionMTC0() -> void;
//};
