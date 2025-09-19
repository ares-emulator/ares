auto ARM7TDMI::idle() -> void {
  endBurst();
  sleep();
}

auto ARM7TDMI::read(u32 mode, n32 address) -> n32 {
  n32 word = get(mode, address);
  nonsequential = false;  //allows burst transfer to continue
  return word;
}

auto ARM7TDMI::load(u32 mode, n32 address) -> n32 {
  endBurst();
  auto word = get(Load | mode, address);
  if(mode & Half) {
    address &= 1;
    word = mode & Signed ? (n32)(i16)word : (n32)(n16)word;
  }
  if(mode & Byte) {
    address &= 0;
    word = mode & Signed ? (n32)(i8)word : (n32)(n8)word;
  }
  if(mode & Signed) {
    word = ASR(word, address.bit(0,1) << 3);
  } else {
    word = ROR(word, address.bit(0,1) << 3);
  }
  return word;
}

auto ARM7TDMI::write(u32 mode, n32 address, n32 word) -> void {
  set(mode, address, word);
  nonsequential = false;  //allows burst transfer to continue
  return;
}

auto ARM7TDMI::store(u32 mode, n32 address, n32 word) -> void {
  endBurst();
  if(mode & Half) { word &= 0xffff; word |= word << 16; }
  if(mode & Byte) { word &= 0xff; word |= word << 8; word |= word << 16; }
  return set(Store | mode, address, word);
}
