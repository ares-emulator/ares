auto DMA::transferLinear(uint c) -> void {
  u32 address = channel[c].address;
  u16 blocks  = channel[c].synchronization == 0 ? uint16(1) : channel[c].blocks;

//print(hex(address, 8L), " ", channel[c].length, " ", blocks, " ", c, " ", channel[c].direction, "\n");

  do {
    u16 length = channel[c].length;
    do {
      if(channel[c].direction == 0) {
        u32 data = 0;
        if(c == 2) {
          data = gpu.readGP0();
        }
        if(c == 3) {
          data = disc.readDMA();
        }
        if(c == 6) {
          data = address - 4 & 0xfffffc;    //point to previous entry
          if(length == 1) data = 0xffffff;  //end of ordering table
        }
        bus.writeWord(address, data);
      }

      if(channel[c].direction == 1) {
        u32 data = bus.readWord(address);
        if(c == 2) {
          gpu.writeGP0(data);
        }
      }

      if(channel[c].step == 0) address += 4;
      if(channel[c].step == 1) address -= 4;
    } while(--length);
  } while(--blocks);
  channel[c].enable  = 0;
  channel[c].trigger = 0;
}

auto DMA::transferLinked(uint c) -> void {
  if(channel[c].direction == 0) return;  //invalid direction for this mode

  u32 address = channel[c].address;
  u32 timeout = 0x40000;  //unverified; prevents potential infinite loop
  do {
    u32 header = bus.readWord(address);

    for(uint count : range(header >> 24)) {
      address += 4;
      u32 data = bus.readWord(address);

      if(c == 2) {
        gpu.writeGP0(data);
      }
    }

    address = header & 0xffffff;
    if(address & 0x800000) break;
  } while(--timeout);
  channel[c].enable  = 0;
  channel[c].trigger = 0;
}
