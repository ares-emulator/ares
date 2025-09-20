
auto Cartridge::KARNAK::power() -> void {
  Thread::create(384'000, std::bind_front(&Cartridge::KARNAK::main, this));
  
  timerEnable = 0;
  timerPeriod = 0;
  timerCounter = 0;
}

auto Cartridge::KARNAK::reset() -> void {
  Thread::destroy();
}

auto Cartridge::KARNAK::main() -> void {
  if(timerEnable) {
    if(!timerCounter) {
      timerCounter = (timerPeriod + 1) * 2;
    }
    timerCounter--;
  }
  cpu.irqLevel(CPU::Interrupt::Cartridge, timerEnable && !timerCounter);
  step(1);
}

auto Cartridge::KARNAK::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

