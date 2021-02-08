auto ARM7TDMI::idle() -> void {
  pipeline.nonsequential = true;
  sleep();
}

auto ARM7TDMI::read(u32 mode, n32 address) -> n32 {
  return get(mode, address);
}

auto ARM7TDMI::load(u32 mode, n32 address) -> n32 {
  pipeline.nonsequential = true;
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
  idle();
  return word;
}

auto ARM7TDMI::write(u32 mode, n32 address, n32 word) -> void {
  pipeline.nonsequential = true;
  return set(mode, address, word);
}

auto ARM7TDMI::store(u32 mode, n32 address, n32 word) -> void {
  pipeline.nonsequential = true;
  if(mode & Half) { word &= 0xffff; word |= word << 16; }
  if(mode & Byte) { word &= 0xff; word |= word << 8; word |= word << 16; }
  return set(Store | mode, address, word);
}
