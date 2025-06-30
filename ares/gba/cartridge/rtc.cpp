auto Cartridge::RTC::irqLevel(bool value) -> void {
  //trigger IRQ on rising edge
  bool irqPrev = irq;
  irq = value;
  if(!irqPrev && irq) cpu.setInterruptFlag(CPU::Interrupt::Cartridge);
}

auto Cartridge::RTC::power() -> void {
  S3511A::power();
  Thread::create(32'768, {&Cartridge::RTC::main, this});
}

auto Cartridge::RTC::main() -> void {
  if(++counter == 0) tickSecond();
  checkAlarm();
  step(1);
}

auto Cartridge::RTC::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}
