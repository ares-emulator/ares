auto POKEY::IRQ::power() -> void {
  enable = 0;
  statusValue = 0xff;
  for(auto& age : enabledAge) age = 0;
  for(auto& age : disabledAge) age = 2;
}

auto POKEY::IRQ::clock() -> void {
  for(u32 source = 0; source < 3; source++) {
    if(enable.bit(source)) {
      enabledAge[source] = std::min(4u, enabledAge[source] + 1);
    } else {
      disabledAge[source] = std::min(2u, disabledAge[source] + 1);
    }
  }
}

auto POKEY::IRQ::request(Source source) -> void {
  auto index = (u32)source;
  if(index < 3) {
    if(enable.bit(index)) {
      if(enabledAge[index] < 4) return;
    } else {
      if(disabledAge[index] >= 2) return;
    }
  } else if(source != SerialComplete && !enable.bit(index)) {
    return;
  }
  statusValue &= ~(1 << index);
}

auto POKEY::IRQ::acknowledge(Source source) -> void {
  statusValue |= 1 << (u32)source;
}

auto POKEY::IRQ::pending(Source source) const -> bool {
  return !statusValue.bit((u32)source);
}

auto POKEY::IRQ::writeEnable(n8 data) -> void {
  bool serialComplete = statusValue.bit(3);
  for(u32 source = 0; source < 3; source++) {
    if(enable.bit(source) == data.bit(source)) continue;
    if(data.bit(source)) enabledAge[source] = 0;
    else disabledAge[source] = 0;
  }
  enable = data;
  statusValue |= ~data;
  statusValue.bit(3) = serialComplete;
}

auto POKEY::IRQ::status() const -> n8 {
  return statusValue;
}

auto POKEY::IRQ::line() const -> bool {
  return ((u8)~statusValue & (u8)enable) != 0;
}
