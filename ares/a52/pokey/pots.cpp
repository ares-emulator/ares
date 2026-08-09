auto POKEY::Pots::power() -> void {
  for(auto& item : value) item = 0xff;
  for(auto& item : target) item = 0;
  allValue = 0x00;
  counter = 0;
  scanning = 0;
}

auto POKEY::Pots::start(const n8 targets[8]) -> void {
  for(auto& item : value) item = 0;
  for(u32 index = 0; index < 8; index++) target[index] = targets[index];
  allValue = 0xff;
  counter = 0;
  scanning = 1;
}

auto POKEY::Pots::clockFast() -> void {
  if(!scanning) return;
  if(counter < FastStop) counter++;
  if(counter <= Maximum) advance();
  if(!scanning) return;

  if(counter == FastMaximum) {
    for(u32 index = 0; index < 8; index++) {
      if(allValue.bit(index)) value[index] = FastMaximum;
    }
  }
  if(counter == FastStop) {
    allValue = 0;
    scanning = 0;
  }
}

auto POKEY::Pots::clock15() -> void {
  if(!scanning) return;
  if(counter < Maximum) counter++;
  advance();
  if(counter != Maximum || !allValue) return;
  for(u32 index = 0; index < 8; index++) {
    if(!allValue.bit(index)) continue;
    value[index] = Maximum;
  }
  allValue = 0;
  scanning = 0;
}

auto POKEY::Pots::advance() -> void {
  for(u32 index = 0; index < 8; index++) {
    if(!allValue.bit(index)) continue;
    if(counter < target[index]) continue;
    value[index] = target[index];
    allValue.bit(index) = 0;
  }
  if(!allValue) scanning = 0;
}

auto POKEY::Pots::read(u32 index) const -> n8 {
  if(allValue.bit(index)) return std::min(counter, FastMaximum);
  return value[index];
}

auto POKEY::Pots::all() const -> n8 {
  return allValue;
}
