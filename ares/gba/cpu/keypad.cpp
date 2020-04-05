auto CPU::Keypad::run() -> void {
  if(!enable) return;
  system.controls.poll();

  const bool lookup[] = {
    system.controls.a->value(),
    system.controls.b->value(),
    system.controls.select->value(),
    system.controls.start->value(),
    system.controls.rightLatch,
    system.controls.leftLatch,
    system.controls.upLatch,
    system.controls.downLatch,
    system.controls.r->value(),
    system.controls.l->value(),
  };

  bool test = condition;  //0 = OR, 1 = AND
  for(uint index : range(10)) {
    if(!flag[index]) continue;
    bool input = lookup[index];
    if(condition == 0) test |= input;
    if(condition == 1) test &= input;
  }
  if(test) cpu.irq.flag |= CPU::Interrupt::Keypad;
}
