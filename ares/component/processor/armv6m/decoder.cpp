auto ARMv6M::decodeInstruction(n16 prefix, maybe<n16> suffix) -> InstructionDecode {
  auto decode = [&](InstructionFamily family, bool valid = true, u32 width = 2) {
    return InstructionDecode{family, width, valid};
  };

  //The order is architectural: some Thumb encodings are proper subsets of
  //later masks.  Execution and disassembly both consume this single list.
  if((prefix & 0xe000) == 0x0000 && (prefix & 0x1800) != 0x1800) return decode(InstructionFamily::ShiftImmediate);
  if((prefix & 0xf800) == 0x1800) return decode(InstructionFamily::AddSubtract);
  if((prefix & 0xe000) == 0x2000) return decode(InstructionFamily::Immediate);
  if((prefix & 0xfc00) == 0x4000) return decode(InstructionFamily::DataProcessing);
  if((prefix & 0xfc00) == 0x4400) return decode(InstructionFamily::HighRegister);
  if((prefix & 0xf800) == 0x4800) return decode(InstructionFamily::LoadLiteral);
  if((prefix & 0xf000) == 0x5000) return decode(InstructionFamily::LoadStoreRegister);
  if((prefix & 0xe000) == 0x6000) return decode(InstructionFamily::LoadStoreImmediate);
  if((prefix & 0xf000) == 0x8000) return decode(InstructionFamily::LoadStoreHalf);
  if((prefix & 0xf000) == 0x9000) return decode(InstructionFamily::LoadStoreStack);
  if((prefix & 0xf000) == 0xa000) return decode(InstructionFamily::AddAddress);
  if((prefix & 0xff00) == 0xb000) return decode(InstructionFamily::AdjustStack);
  if((prefix & 0xffe8) == 0xb660) return decode(InstructionFamily::ChangeInterrupt);
  if(prefix == 0xb650 || prefix == 0xb658) return decode(InstructionFamily::SetEndian, prefix == 0xb650);
  if((prefix & 0xff00) == 0xb200) return decode(InstructionFamily::Extend);
  if((prefix & 0xfe00) == 0xb400) return decode(InstructionFamily::Push);
  if((prefix & 0xff00) == 0xba00) return decode(InstructionFamily::Reverse, (prefix >> 6 & 3) != 2);
  if((prefix & 0xff00) == 0xbe00) return decode(InstructionFamily::Breakpoint);
  if((prefix & 0xff00) == 0xbf00) {
    auto mask = prefix & 15;
    auto condition = prefix >> 4 & 15;
    return decode(InstructionFamily::Hint, !mask || condition < 14);
  }
  if((prefix & 0xfe00) == 0xbc00) return decode(InstructionFamily::Pop);
  if((prefix & 0xf000) == 0xc000) return decode(InstructionFamily::LoadStoreMultiple, (prefix & 0xff) != 0);
  if((prefix & 0xf000) == 0xd000) return decode(InstructionFamily::BranchConditional, (prefix >> 8 & 15) < 14);
  if((prefix & 0xf800) == 0xe000) return decode(InstructionFamily::Branch);
  if((prefix & 0xf800) == 0xf000) {
    //Without a suffix this still identifies a four-byte family.  Supplying a
    //suffix additionally validates the ARMv6-M BL second-halfword pattern.
    auto valid = !suffix || ((*suffix & 0xd000) == 0xd000);
    return decode(InstructionFamily::BranchLink, valid, 4);
  }
  return decode(InstructionFamily::Undefined, false);
}
