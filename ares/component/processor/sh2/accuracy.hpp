struct Accuracy {
  //enable all accuracy flags
  static constexpr bool Reference = 0;

  static constexpr bool Interpreter = 0 | Reference;
  static constexpr bool Recompiler = !Interpreter;

  //exceptions when the CPU or DMA accesses unaligned memory addresses
  //unemulated: access of Purge, Address, or IO areas by PC-relative addressing
  //unemulated: address errors caused by stacking of address error exception handling
  static constexpr bool AddressErrors = 0 | Reference & !Recompiler;
};
