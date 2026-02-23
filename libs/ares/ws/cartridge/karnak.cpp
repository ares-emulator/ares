static const n10 adpcmAccumulatorStep[16][16] = {
  { 0,  0,  1,  2,  3,   5,   7,  10,  0,   0,  -1,  -2,  -3,   -5,   -7,  -10 },
  { 0,  1,  2,  3,  4,   6,   8,  13,  0,  -1,  -2,  -3,  -4,   -6,   -8,  -13 },
  { 0,  1,  2,  4,  5,   7,  10,  15,  0,  -1,  -2,  -4,  -5,   -7,  -10,  -15 },
  { 0,  1,  3,  4,  6,   9,  13,  19,  0,  -1,  -3,  -4,  -6,   -9,  -13,  -19 },
  { 0,  2,  3,  5,  8,  11,  15,  23,  0,  -2,  -3,  -5,  -8,  -11,  -15,  -23 },
  { 0,  2,  4,  7, 10,  14,  19,  29,  0,  -2,  -4,  -7, -10,  -14,  -19,  -29 },
  { 0,  3,  5,  8, 12,  16,  22,  33,  0,  -3,  -5,  -8, -12,  -16,  -22,  -33 },
  { 1,  4,  7, 10, 15,  20,  29,  43, -1,  -4,  -7, -10, -15,  -20,  -29,  -43 },
  { 1,  4,  8, 13, 18,  25,  35,  53, -1,  -4,  -8, -13, -18,  -25,  -35,  -53 },
  { 1,  6, 10, 16, 22,  31,  43,  64, -1,  -6, -10, -16, -22,  -31,  -43,  -64 },
  { 2,  7, 12, 19, 27,  37,  51,  76, -2,  -7, -12, -19, -27,  -37,  -51,  -76 },
  { 2,  9, 16, 24, 34,  46,  64,  96, -2,  -9, -16, -24, -34,  -46,  -64,  -96 },
  { 3, 11, 19, 29, 41,  57,  79, 117, -3, -11, -19, -29, -41,  -57,  -79, -117 },
  { 4, 13, 24, 36, 50,  69,  96, 143, -4, -13, -24, -36, -50,  -69,  -96, -143 },
  { 4, 16, 29, 44, 62,  85, 118, 175, -4, -16, -29, -44, -62,  -85, -118, -175 },
  { 6, 20, 36, 54, 76, 104, 144, 214, -6, -20, -36, -54, -76, -104, -144, -214 }
};
static const i4 adpcmIndexStep[8] = {
  -1, -1, 0, 0, 1, 2, 2, 3
};

auto Cartridge::KARNAK::power() -> void {
  Thread::create(384'000, std::bind_front(&Cartridge::KARNAK::main, this));
  
  enable = 0;
  timerPeriod = 0;
  timerCounter = 0;

  adpcmReset();
}

auto Cartridge::KARNAK::reset() -> void {
  Thread::destroy();
}

auto Cartridge::KARNAK::adpcmReset() -> void {
  adpcmAccumulator = 0x100;
  adpcmInputShift = 0;
  adpcmStepIndex = 0;
}

auto Cartridge::KARNAK::adpcmNext(n4 sample) -> void {
  adpcmAccumulator += adpcmAccumulatorStep[adpcmStepIndex][sample];
  adpcmStepIndex = std::clamp(adpcmStepIndex + adpcmIndexStep[sample & 7], 0, 15);
}

auto Cartridge::KARNAK::main() -> void {
  if(enable) {
    if(!timerCounter) {
      timerCounter = (timerPeriod + 1) * 2;
    }
    timerCounter--;
  }
  cpu.irqLevel(CPU::Interrupt::Cartridge, enable && !timerCounter);
  step(1);
}

auto Cartridge::KARNAK::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

