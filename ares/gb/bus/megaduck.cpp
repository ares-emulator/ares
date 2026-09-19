//the Mega Duck is DMG hardware with its I/O window rewired. the CPU, PPU and
//APU are unchanged, so rather than duplicate their decoders the address is
//translated back to the Game Boy layout before the bus dispatches it, and the
//few registers whose bits or nibbles were also shuffled are corrected here.
//
//  Mega Duck   Game Boy   contents
//  ff00-ff0f   ff00-ff0f  joypad, serial, timer, interrupt flag (unchanged)
//  ff10-ff1f   ff40-ff4f  video registers, reordered within the window
//  ff20-ff2f   ff10-ff1f  sound channels 1-3, reordered within the window
//  ff30-ff3f   ff30-ff3f  wave pattern RAM (unchanged)
//  ff40-ff46   ff20-ff26  noise channel and master volume, reordered
//  ff47-ff7f   unmapped
//
//taken from MAME's megaduck_state, and corroborated against the cartridges:
//brckwall writes 0xe4 to ff1b during setup, which is the standard background
//palette value, and ff1b is where the table below puts BGP.

//sound registers are swapped in pairs: 1<->2 and 5<->6 within each window
static const n8 MegaDuckSoundOffsets[16] = {
  0x0, 0x2, 0x1, 0x3, 0x4, 0x6, 0x5, 0x7,
  0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
};

auto Bus::megaDuckAddress(n16 address) -> maybe<n16> {
  if(address >= 0xff10 && address <= 0xff1f) {
    //video: the middle two quarters of the window trade places
    n8 offset = address & 0x0f;
    if((offset & 0x0c) && (offset & 0x0c) != 0x0c) offset ^= 0x0c;
    return n16(0xff40 | offset);
  }

  if(address >= 0xff20 && address <= 0xff2f) {
    return n16(0xff10 | MegaDuckSoundOffsets[address & 0x0f]);
  }

  if(address >= 0xff40 && address <= 0xff46) {
    return n16(0xff20 | MegaDuckSoundOffsets[address & 0x0f]);
  }

  if(address >= 0xff47 && address <= 0xff7f) {
    return nothing;  //unmapped: reads float high, writes are discarded
  }

  return address;
}

//LCDC keeps its meaning but not its bit order:
//  Game Boy bit  0  1  2  3  4  5  6  7
//  Mega Duck bit 6  0  1  2  4  5  3  7
auto Bus::megaDuckWriteData(n16 address, n8 data) -> n8 {
  if(address == 0xff10) {  //LCDC
    n8 value;
    value.bit(0) = data.bit(6);
    value.bit(1) = data.bit(0);
    value.bit(2) = data.bit(1);
    value.bit(3) = data.bit(2);
    value.bit(4) = data.bit(4);
    value.bit(5) = data.bit(5);
    value.bit(6) = data.bit(3);
    value.bit(7) = data.bit(7);
    return value;
  }

  //the envelope and noise registers have their nibbles reversed
  if(address == 0xff21 || address == 0xff27) return data >> 4 | data << 4;
  if(address == 0xff41 || address == 0xff42) return data >> 4 | data << 4;

  return data;
}

auto Bus::megaDuckReadData(n16 address, n8 data) -> n8 {
  if(address == 0xff10) {  //LCDC, inverse of the write mapping above
    n8 value;
    value.bit(0) = data.bit(1);
    value.bit(1) = data.bit(2);
    value.bit(2) = data.bit(3);
    value.bit(3) = data.bit(6);
    value.bit(4) = data.bit(4);
    value.bit(5) = data.bit(5);
    value.bit(6) = data.bit(0);
    value.bit(7) = data.bit(7);
    return value;
  }

  if(address == 0xff21 || address == 0xff27) return data >> 4 | data << 4;
  if(address == 0xff41 || address == 0xff42) return data >> 4 | data << 4;

  return data;
}
