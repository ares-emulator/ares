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

  bool inputHasChanged = false;
  conditionMet = condition;  //0 = OR, 1 = AND
  for(u32 index : range(10)) {
    if(lookup[index] != keyStates[index]) inputHasChanged = true;
    keyStates[index] = lookup[index];
    if(!flag[index]) continue;
    n1 input = lookup[index];
    if(condition == 0) conditionMet |= input;
    if(condition == 1) conditionMet &= input;
  }
  
  if(inputHasChanged && conditionMet && enable) cpu.setInterruptFlag(CPU::Interrupt::Keypad);
}
