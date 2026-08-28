auto Harmony::sleep() -> void {
  step(1);
}

auto Harmony::get(u32 mode, n32 address) -> n32 {
  step(1);
  u32 size = mode & Byte ? 1 : mode & Half ? 2 : 4;
  u32 location = address;
  if((size == 2 && (location & 1)) || (size == 4 && (location & 3))) {
    faulted = true;
    return 0;
  }

  n32 data = 0;
  auto access = cartridge.readARM(mode, address, data);
  if(access == Access::Granted) return data;
  if(access == Access::Fault) {
    faulted = true;
    return 0;
  }

  if(location == 0xe0008004) return timer1Control;
  if(location == 0xe0008008) return timer1Counter;
  if(location == 0xe000e010) {
    auto data = systickControl;
    systickControl &= ~0x00010000;
    return data;
  }
  if(location == 0xe000e014) return systickReload;
  if(location == 0xe000e018) return systickCounter;
  if(location == 0xe000e01c) return systickCalibration;
  if(location == 0xe01fc000) return mamControl;
  if(location == 0xe01fc100) return 1;
  faulted = true;
  return 0;
}

auto Harmony::set(u32 mode, n32 address, n32 word) -> void {
  step(1);
  u32 size = mode & Byte ? 1 : mode & Half ? 2 : 4;
  u32 location = address;
  if((size == 2 && (location & 1)) || (size == 4 && (location & 3))) {
    faulted = true;
    return;
  }

  auto access = cartridge.writeARM(mode, address, word);
  if(access == Access::Granted) return;
  if(access == Access::Fault) {
    faulted = true;
    return;
  }

  if(location == 0xe0000000) {}
  else if(location == 0xe0008004) timer1Control = word;
  else if(location == 0xe0008008) timer1Counter = word;
  else if(location == 0xe000e010) {
    if(!(systickControl & 1) && (word & 1)) systickCounter = systickReload;
    systickControl = word & 0x00010007;
  }
  else if(location == 0xe000e014) systickReload = word & 0x00ffffff;
  else if(location == 0xe000e018) systickCounter = word & 0x00ffffff;
  else if(location == 0xe000e01c) systickCalibration = word & 0x00ffffff;
  else if(location == 0xe01fc000) mamControl = word;
  else if(location == 0xf0000000) faulted = true;
  else if(location >= 0xe0000000 && location < 0xf0000000) {}
  else {
    faulted = true;
  }
}
