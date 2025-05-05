auto APU::Wave::run() -> void {
  if(period && --period == 0) {
    period = 1 * (2048 - frequency);
    patternsample = pattern[patternbank << 5 | patternaddr++];
    if(patternaddr == 0) patternbank ^= mode;
  }

  output = patternsample;
  static u32 multiplier[] = {0, 4, 2, 1, 3, 3, 3, 3};
  output = (output * multiplier[volume]) / 4;
  if(enable == false) output = 0;
}

auto APU::Wave::clockLength() -> void {
  if(enable && counter) {
    if(++length == 0) enable = false;
  }
}

auto APU::Wave::read(u32 address) const -> n8 {
  switch(address) {
  case 0: return (mode << 5) | (bank << 6) | (dacenable << 7);
  case 1: return 0;
  case 2: return (volume << 5);
  case 3: return 0;
  case 4: return (counter << 6);
  }
  return 0;
}

auto APU::Wave::write(u32 address, n8 byte) -> void {
  switch(address) {
  case 0:  //NR30
    mode      = byte >> 5;
    bank      = byte >> 6;
    dacenable = byte >> 7;
    if(dacenable == false) enable = false;
    break;

  case 1:  //NR31
    length = byte >> 0;
    break;

  case 2:  //NR32
    volume = byte >> 5;
    break;

  case 3:  //NR33
    frequency = (frequency & 0xff00) | (byte << 0);
    break;

  case 4:  //NR34
    frequency = (frequency & 0x00ff) | (byte << 8);
    counter    = byte >> 6;
    initialize = byte >> 7;

    if(initialize) {
      enable = dacenable;
      period = 1 * (2048 - frequency);
      patternaddr = 0;
      patternbank = mode ? (n1)0 : bank;
    }

    break;
  }
}

auto APU::Wave::freeBank() const -> n1 {
  return bank && apu.sequencer.masterenable;
}

auto APU::Wave::readRAM(u32 address) const -> n8 {
  n8 byte = 0;
  byte |= pattern[freeBank() << 5 | address << 1 | 0] << 4;
  byte |= pattern[freeBank() << 5 | address << 1 | 1] << 0;
  return byte;
}

auto APU::Wave::writeRAM(u32 address, n8 byte) -> void {
  pattern[freeBank() << 5 | address << 1 | 0] = byte >> 4;
  pattern[freeBank() << 5 | address << 1 | 1] = byte >> 0;
}

auto APU::Wave::power() -> void {
  mode = 0;
  bank = 0;
  dacenable = 0;
  length = 0;
  volume = 0;
  frequency = 0;
  counter = 0;
  initialize = 0;
  for(auto& sample : pattern) sample = 0;
  enable = 0;
  output = 0;
  patternaddr = 0;
  patternbank = 0;
  patternsample = 0;
  period = 0;
}
