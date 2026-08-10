auto ANTIC::read(n8 address) -> n8 {
  return peek(address);
}

auto ANTIC::peek(n8 address) const -> n8 {
  switch(address & 0x0f) {
  case 0x0b: {
    // AHRM 4.10: $83 is visible only on cycle 111 of the final NTSC line.
    if(counter.scanline == Timing::ScanlinesPerFrame - 1 && counter.machineCycle >= 112) return 0;
    auto scanline = (u16)counter.scanline + ((u8)counter.machineCycle >= 111);
    return scanline >> 1;
  }
  case 0x0c: return io.penh;
  case 0x0d: return io.penv;
  case 0x0f: return io.nmist | 0x1f;
  }
  return 0xff;
}

auto ANTIC::write(n8 address, n8 data) -> void {
  switch(address & 0x0f) {
  case 0x00: io.dmactl = data;        return;
  case 0x01: io.chactl = data;        return;
  case 0x02: io.dlist.byte(0) = data; return;
  case 0x03: io.dlist.byte(1) = data; return;
  case 0x04: io.hscroll = data;       return;
  case 0x05: io.vscroll = data;       return;
  case 0x07: io.pmbase = data;        return;
  case 0x09: io.chbase = data;        return;
  case 0x0a: wsync.wait();            return;
  case 0x0e: io.nmien = data;         return;
  case 0x0f: io.nmist = 0;            return;
  }
}
