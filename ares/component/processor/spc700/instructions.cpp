auto SPC700::instructionAbsoluteBitModify(n3 mode) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  n3 bit = address >> 13;
  address &= 0x1fff;
  n8 data = read(address);
  switch(mode) {
  case 0:  //or addr:bit
    idle();
    CF |= data.bit(bit);
    break;
  case 1:  //or !addr:bit
    idle();
    CF |= !data.bit(bit);
    break;
  case 2:  //and addr:bit
    CF &= data.bit(bit);
    break;
  case 3:  //and !addr:bit
    CF &= !data.bit(bit);
    break;
  case 4:  //eor addr:bit
    idle();
    CF ^= data.bit(bit);
    break;
  case 5:  //ld addr:bit
    CF = data.bit(bit);
    break;
  case 6:  //st addr:bit
    idle();
    data.bit(bit) = CF;
    write(address, data);
    break;
  case 7:  //not addr:bit
    data.bit(bit) ^= 1;
    write(address, data);
    break;
  }
}

auto SPC700::instructionAbsoluteBitSet(n3 bit, bool value) -> void {
  n8 address = fetch();
  n8 data = load(address);
  data.bit(bit) = value;
  store(address, data);
}

auto SPC700::instructionAbsoluteRead(fpb op, n8& target) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  n8 data = read(address);
  target = alu(target, data);
}

auto SPC700::instructionAbsoluteModify(fps op) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  n8 data = read(address);
  write(address, alu(data));
}

auto SPC700::instructionAbsoluteWrite(n8& data) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  read(address);
  write(address, data);
}

auto SPC700::instructionAbsoluteIndexedRead(fpb op, n8& index) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  idle();
  n8 data = read(address + index);
  A = alu(A, data);
}

auto SPC700::instructionAbsoluteIndexedWrite(n8& index) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  idle();
  read(address + index);
  write(address + index, A);
}

auto SPC700::instructionBranch(bool take) -> void {
  n8 data = fetch();
  if(!take) return;
  idle();
  idle();
  PC += (i8)data;
}

auto SPC700::instructionBranchBit(n3 bit, bool match) -> void {
  n8 address = fetch();
  n8 data = load(address);
  idle();
  n8 displacement = fetch();
  if(data.bit(bit) != match) return;
  idle();
  idle();
  PC += (i8)displacement;
}

auto SPC700::instructionBranchNotDirect() -> void {
  n8 address = fetch();
  n8 data = load(address);
  idle();
  n8 displacement = fetch();
  if(A == data) return;
  idle();
  idle();
  PC += (i8)displacement;
}

auto SPC700::instructionBranchNotDirectDecrement() -> void {
  n8 address = fetch();
  n8 data = load(address);
  store(address, --data);
  n8 displacement = fetch();
  if(data == 0) return;
  idle();
  idle();
  PC += (i8)displacement;
}

auto SPC700::instructionBranchNotDirectIndexed(n8& index) -> void {
  n8 address = fetch();
  idle();
  n8 data = load(address + index);
  idle();
  n8 displacement = fetch();
  if(A == data) return;
  idle();
  idle();
  PC += (i8)displacement;
}

auto SPC700::instructionBranchNotYDecrement() -> void {
  read(PC);
  idle();
  n8 displacement = fetch();
  if(--Y == 0) return;
  idle();
  idle();
  PC += (i8)displacement;
}

auto SPC700::instructionBreak() -> void {
  read(PC);
  push(PC >> 8);
  push(PC >> 0);
  push(P);
  idle();
  n16 address = read(0xffde + 0);
  address |= read(0xffde + 1) << 8;
  PC = address;
  IF = 0;
  BF = 1;
}

auto SPC700::instructionCallAbsolute() -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  idle();
  push(PC >> 8);
  push(PC >> 0);
  idle();
  idle();
  PC = address;
}

auto SPC700::instructionCallPage() -> void {
  n8 address = fetch();
  idle();
  push(PC >> 8);
  push(PC >> 0);
  idle();
  PC = 0xff00 | address;
}

auto SPC700::instructionCallTable(n4 vector) -> void {
  read(PC);
  idle();
  push(PC >> 8);
  push(PC >> 0);
  idle();
  n16 address = 0xffde - (vector << 1);
  n16 pc = read(address + 0);
  pc |= read(address + 1) << 8;
  PC = pc;
}

auto SPC700::instructionComplementCarry() -> void {
  read(PC);
  idle();
  CF = !CF;
}

auto SPC700::instructionDecimalAdjustAdd() -> void {
  read(PC);
  idle();
  if(CF || A > 0x99) {
    A += 0x60;
    CF = 1;
  }
  if(HF || (A & 15) > 0x09) {
    A += 0x06;
  }
  ZF = A == 0;
  NF = A & 0x80;
}

auto SPC700::instructionDecimalAdjustSub() -> void {
  read(PC);
  idle();
  if(!CF || A > 0x99) {
    A -= 0x60;
    CF = 0;
  }
  if(!HF || (A & 15) > 0x09) {
    A -= 0x06;
  }
  ZF = A == 0;
  NF = A & 0x80;
}

auto SPC700::instructionDirectRead(fpb op, n8& target) -> void {
  n8 address = fetch();
  n8 data = load(address);
  target = alu(target, data);
}

auto SPC700::instructionDirectModify(fps op) -> void {
  n8 address = fetch();
  n8 data = load(address);
  store(address, alu(data));
}

auto SPC700::instructionDirectWrite(n8& data) -> void {
  n8 address = fetch();
  load(address);
  store(address, data);
}

auto SPC700::instructionDirectDirectCompare(fpb op) -> void {
  n8 source = fetch();
  n8 rhs = load(source);
  n8 target = fetch();
  n8 lhs = load(target);
  lhs = alu(lhs, rhs);
  idle();
}

auto SPC700::instructionDirectDirectModify(fpb op) -> void {
  n8 source = fetch();
  n8 rhs = load(source);
  n8 target = fetch();
  n8 lhs = load(target);
  lhs = alu(lhs, rhs);
  store(target, lhs);
}

auto SPC700::instructionDirectDirectWrite() -> void {
  n8 source = fetch();
  n8 data = load(source);
  n8 target = fetch();
  store(target, data);
}

auto SPC700::instructionDirectImmediateCompare(fpb op) -> void {
  n8 immediate = fetch();
  n8 address = fetch();
  n8 data = load(address);
  data = alu(data, immediate);
  idle();
}

auto SPC700::instructionDirectImmediateModify(fpb op) -> void {
  n8 immediate = fetch();
  n8 address = fetch();
  n8 data = load(address);
  data = alu(data, immediate);
  store(address, data);
}

auto SPC700::instructionDirectImmediateWrite() -> void {
  n8 immediate = fetch();
  n8 address = fetch();
  load(address);
  store(address, immediate);
}

auto SPC700::instructionDirectCompareWord(fpw op) -> void {
  n8 address = fetch();
  n16 data = load(address + 0);
  data |= load(address + 1) << 8;
  YA = alu(YA, data);
}

auto SPC700::instructionDirectReadWord(fpw op) -> void {
  n8 address = fetch();
  n16 data = load(address + 0);
  idle();
  data |= load(address + 1) << 8;
  YA = alu(YA, data);
}

auto SPC700::instructionDirectModifyWord(s32 adjust) -> void {
  n8 address = fetch();
  n16 data = load(address + 0) + adjust;
  store(address + 0, data >> 0);
  data += load(address + 1) << 8;
  store(address + 1, data >> 8);
  ZF = data == 0;
  NF = data & 0x8000;
}

auto SPC700::instructionDirectWriteWord() -> void {
  n8 address = fetch();
  load(address + 0);
  store(address + 0, A);
  store(address + 1, Y);
}

auto SPC700::instructionDirectIndexedRead(fpb op, n8& target, n8& index) -> void {
  n8 address = fetch();
  idle();
  n8 data = load(address + index);
  target = alu(target, data);
}

auto SPC700::instructionDirectIndexedModify(fps op, n8& index) -> void {
  n8 address = fetch();
  idle();
  n8 data = load(address + index);
  store(address + index, alu(data));
}

auto SPC700::instructionDirectIndexedWrite(n8& data, n8& index) -> void {
  n8 address = fetch();
  idle();
  load(address + index);
  store(address + index, data);
}

auto SPC700::instructionDivide() -> void {
  read(PC);
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  n16 ya = YA;
  //overflow set if quotient >= 256
  HF = (Y & 15) >= (X & 15);
  VF = Y >= X;
  if(Y < (X << 1)) {
    //if quotient is <= 511 (will fit into 9-bit result)
    A = ya / X;
    Y = ya % X;
  } else {
    //otherwise, the quotient won't fit into VF + A
    //this emulates the odd behavior of the S-SMP in this case
    A = 255 - (ya - (X << 9)) / (256 - X);
    Y = X   + (ya - (X << 9)) % (256 - X);
  }
  //result is set based on a (quotient) only
  ZF = A == 0;
  NF = A & 0x80;
}

auto SPC700::instructionExchangeNibble() -> void {
  read(PC);
  idle();
  idle();
  idle();
  A = A >> 4 | A << 4;
  ZF = A == 0;
  NF = A & 0x80;
}

auto SPC700::instructionFlagSet(bool& flag, bool value) -> void {
  read(PC);
  if(&flag == &IF) idle();
  flag = value;
}

auto SPC700::instructionImmediateRead(fpb op, n8& target) -> void {
  n8 data = fetch();
  target = alu(target, data);
}

auto SPC700::instructionImpliedModify(fps op, n8& target) -> void {
  read(PC);
  target = alu(target);
}

auto SPC700::instructionIndexedIndirectRead(fpb op, n8& index) -> void {
  n8 indirect = fetch();
  idle();
  n16 address = load(indirect + index + 0);
  address |= load(indirect + index + 1) << 8;
  n8 data = read(address);
  A = alu(A, data);
}

auto SPC700::instructionIndexedIndirectWrite(n8& data, n8& index) -> void {
  n8 indirect = fetch();
  idle();
  n16 address = load(indirect + index + 0);
  address |= load(indirect + index + 1) << 8;
  read(address);
  write(address, data);
}

auto SPC700::instructionIndirectIndexedRead(fpb op, n8& index) -> void {
  n8 indirect = fetch();
  idle();
  n16 address = load(indirect + 0);
  address |= load(indirect + 1) << 8;
  n8 data = read(address + index);
  A = alu(A, data);
}

auto SPC700::instructionIndirectIndexedWrite(n8& data, n8& index) -> void {
  n8 indirect = fetch();
  n16 address = load(indirect + 0);
  address |= load(indirect + 1) << 8;
  idle();
  read(address + index);
  write(address + index, data);
}

auto SPC700::instructionIndirectXRead(fpb op) -> void {
  read(PC);
  n8 data = load(X);
  A = alu(A, data);
}

auto SPC700::instructionIndirectXWrite(n8& data) -> void {
  read(PC);
  load(X);
  store(X, data);
}

auto SPC700::instructionIndirectXIncrementRead(n8& data) -> void {
  read(PC);
  data = load(X++);
  idle();  //quirk: consumes extra idle cycle compared to most read instructions
  ZF = data == 0;
  NF = data & 0x80;
}

auto SPC700::instructionIndirectXIncrementWrite(n8& data) -> void {
  read(PC);
  idle();  //quirk: not a read cycle as with most write instructions
  store(X++, data);
}

auto SPC700::instructionIndirectXCompareIndirectY(fpb op) -> void {
  read(PC);
  n8 rhs = load(Y);
  n8 lhs = load(X);
  lhs = alu(lhs, rhs);
  idle();
}

auto SPC700::instructionIndirectXWriteIndirectY(fpb op) -> void {
  read(PC);
  n8 rhs = load(Y);
  n8 lhs = load(X);
  lhs = alu(lhs, rhs);
  store(X, lhs);
}

auto SPC700::instructionJumpAbsolute() -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  PC = address;
}

auto SPC700::instructionJumpIndirectX() -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  idle();
  n16 pc = read(address + X + 0);
  pc |= read(address + X + 1) << 8;
  PC = pc;
}

auto SPC700::instructionMultiply() -> void {
  read(PC);
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  idle();
  n16 ya = Y * A;
  A = ya >> 0;
  Y = ya >> 8;
  //result is set based on y (high-byte) only
  ZF = Y == 0;
  NF = Y & 0x80;
}

auto SPC700::instructionNoOperation() -> void {
  read(PC);
}

auto SPC700::instructionOverflowClear() -> void {
  read(PC);
  HF = 0;
  VF = 0;
}

auto SPC700::instructionPull(n8& data) -> void {
  read(PC);
  idle();
  data = pull();
}

auto SPC700::instructionPullP() -> void {
  read(PC);
  idle();
  P = pull();
}

auto SPC700::instructionPush(n8 data) -> void {
  read(PC);
  push(data);
  idle();
}

auto SPC700::instructionReturnInterrupt() -> void {
  read(PC);
  idle();
  P = pull();
  n16 address = pull();
  address |= pull() << 8;
  PC = address;
}

auto SPC700::instructionReturnSubroutine() -> void {
  read(PC);
  idle();
  n16 address = pull();
  address |= pull() << 8;
  PC = address;
}

auto SPC700::instructionStop() -> void {
  r.stop = true;
  while(r.stop && !synchronizing()) {
    read(PC);
    idle();
  }
}

auto SPC700::instructionTestSetBitsAbsolute(bool set) -> void {
  n16 address = fetch();
  address |= fetch() << 8;
  n8 data = read(address);
  ZF = (A - data) == 0;
  NF = (A - data) & 0x80;
  read(address);
  write(address, set ? data | A : data & ~A);
}

auto SPC700::instructionTransfer(n8& from, n8& to) -> void {
  read(PC);
  to = from;
  if(&to == &S) return;
  ZF = to == 0;
  NF = to & 0x80;
}

auto SPC700::instructionWait() -> void {
  r.wait = true;
  while(r.wait && !synchronizing()) {
    read(PC);
    idle();
  }
}
