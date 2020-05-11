//{
  struct Exception {
    CPU& self;
    auto tlbModification() { self.raiseException(1); }
    auto tlbLoad() { self.raiseException(2); }
    auto tlbStore() { self.raiseException(3); }
    auto addressLoad() { self.raiseException(4); }
    auto addressStore() { self.raiseException(5); }
    auto busInstruction() { self.raiseException(6); }
    auto busData() { self.raiseException(7); }
    auto systemCall() { self.raiseException(8); }
    auto breakpoint() { self.raiseException(9); }
    auto reservedInstruction() { self.raiseException(10); }
    auto coprocessor0() { self.raiseException(11, 0); }
    auto coprocessor1() { self.raiseException(11, 1); }
    auto coprocessor2() { self.raiseException(11, 2); }
    auto coprocessor3() { self.raiseException(11, 3); }
    auto arithmeticOverflow() { self.raiseException(12); }
    auto trap() { self.raiseException(13); }
    auto floatingPoint() { self.raiseException(15); }
    auto watchAddress() { self.raiseException(23); }
  } exception{*this};
//};
