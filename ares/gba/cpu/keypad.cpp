auto CPU::Keypad::run() -> void {
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

  conditionMet = condition;  //0 = OR, 1 = AND
  for(u32 index : range(10)) {
    if(lookup[index] != keyStates[index]) irq = false;  //force IRQ line low temporarily when key state changes
    keyStates[index] = lookup[index];
    if(!flag[index]) continue;
    n1 input = lookup[index];
    if(condition == 0) conditionMet |= input;
    if(condition == 1) conditionMet &= input;
  }
  bool irqPrev = irq;
  irq = conditionMet && enable;
  if(irq && !irqPrev) cpu.setInterruptFlag(CPU::Interrupt::Keypad);
}
