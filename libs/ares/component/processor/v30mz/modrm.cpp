auto V30MZ::modRM(bool forceAddress) -> void {
  auto data = fetch<Byte>();
  modrm.mem = data >> 0 & 7;
  modrm.reg = data >> 3 & 7;
  modrm.mod = data >> 6 & 3;
  modrm.useAddress = modrm.mod != 3;

  if(modrm.mod == 0 && modrm.mem == 6) {
    modrm.segment = segment(DS0);
    modrm.address = fetch<Word>();
  } else if (modrm.mod == 3) {
    if (!forceAddress) return;
    // This is used when LEA/LDS/LES is called with register addressing mode,
    // which is normally undefined behaviour.
    //
    // TODO: Does this incur a 1-clock penalty? When?
    modrm.useAddress = true;
    wait(1);
    switch(modrm.mem) {
    case 0: modrm.segment = segment(DS0); modrm.address = BW + AW; break;
    case 1: modrm.segment = segment(DS0); modrm.address = BW + CW; break;
    case 2: modrm.segment = segment(SS ); modrm.address = BP + DW; break;
    case 3: modrm.segment = segment(SS ); modrm.address = BP + BW; break;
    case 4: modrm.segment = segment(DS0); modrm.address = IX + SP; break;
    case 5: modrm.segment = segment(DS0); modrm.address = IY + BP; break;
    case 6: modrm.segment = segment(SS ); modrm.address = BP + IX; break;
    case 7: modrm.segment = segment(DS0); modrm.address = BW + IY; break;
    }
  } else {
    switch(modrm.mem) {
    case 0: modrm.segment = segment(DS0); modrm.address = BW + IX; wait(1); break;
    case 1: modrm.segment = segment(DS0); modrm.address = BW + IY; wait(1); break;
    case 2: modrm.segment = segment(SS ); modrm.address = BP + IX; wait(1); break;
    case 3: modrm.segment = segment(SS ); modrm.address = BP + IY; wait(1); break;
    case 4: modrm.segment = segment(DS0); modrm.address = IX; break;
    case 5: modrm.segment = segment(DS0); modrm.address = IY; break;
    case 6: modrm.segment = segment(SS ); modrm.address = BP; break;
    case 7: modrm.segment = segment(DS0); modrm.address = BW; break;
    }
    if(modrm.mod == 1) modrm.address += ( i8)fetch<Byte>();
    if(modrm.mod == 2) modrm.address += (i16)fetch<Word>();
  }
}

auto V30MZ::getSegment() -> u16 { return *RS[modrm.reg]; }

auto V30MZ::setSegment(u16 data) -> void { *RS[modrm.reg] = data; }

template<> auto V30MZ::getRegister<Byte>() -> u16 { return *RB[modrm.reg]; }
template<> auto V30MZ::getRegister<Word>() -> u16 { return *RW[modrm.reg]; }

template<> auto V30MZ::setRegister<Byte>(u16 data) -> void { *RB[modrm.reg] = data; }
template<> auto V30MZ::setRegister<Word>(u16 data) -> void { *RW[modrm.reg] = data; }

template<> auto V30MZ::getMemory<Byte>(u32 offset) -> u16 {
  if(!modrm.useAddress) return *RB[modrm.mem];
  return read<Byte>(modrm.segment, modrm.address + offset);
}

template<> auto V30MZ::getMemory<Word>(u32 offset) -> u16 {
  if(!modrm.useAddress) return *RW[modrm.mem];
  return read<Word>(modrm.segment, modrm.address + offset);
}

template<> auto V30MZ::setMemory<Byte>(u16 data) -> void {
  if(!modrm.useAddress) return (void)(*RB[modrm.mem] = data);
  return write<Byte>(modrm.segment, modrm.address, data);
}

template<> auto V30MZ::setMemory<Word>(u16 data) -> void {
  if(!modrm.useAddress) return (void)(*RW[modrm.mem] = data);
  return write<Word>(modrm.segment, modrm.address, data);
}
