auto DMA::transferLinear(uint c) -> void {
  channel[c].trigger = 0;

  u32 address = channel[c].address;
  u16 blocks  = channel[c].synchronization == 0 ? uint16(1) : channel[c].blocks;

//print(hex(address, 8L), " ", channel[c].length, " ", blocks, " ", c, " ", channel[c].direction, "\n");

  do {
    u16 length = channel[c].length;
    do {
      if(channel[c].direction == 0) {
        u32 data = 0;
        if(c == 0) {
          debug(unimplemented, "DMA MDECin read");
        }
        if(c == 1) {
          debug(unimplemented, "DMA MDECout read");
        }
        if(c == 2) {
          data = gpu.readGP0();
        }
        if(c == 3) {
          data = disc.readDMA();
        }
        if(c == 4) {
          data = spu.readDMA();
        }
        if(c == 5) {
          debug(unimplemented, "DMA PIO read");
        }
        if(c == 6) {
          data = address - 4 & 0xfffffc;    //point to previous entry
          if(length == 1) data = 0xffffff;  //end of ordering table
        }
        bus.writeWord(address, data);
      }

      if(channel[c].direction == 1) {
        u32 data = bus.readWord(address);
        if(c == 0) {
          debug(unimplemented, "DMA MDECin write");
        }
        if(c == 1) {
          debug(unimplemented, "DMA MDECout write");
        }
        if(c == 2) {
          gpu.writeGP0(data);
        }
        if(c == 3) {
          debug(unimplemented, "DMA CDROM write");
        }
        if(c == 4) {
          spu.writeDMA(data);
        }
        if(c == 5) {
          debug(unimplemented, "DMA PIO write");
        }
        if(c == 6) {
          debug(unimplemented, "DMA OTC write");
        }
      }

      if(channel[c].step == 0) address += 4;
      if(channel[c].step == 1) address -= 4;
    } while(--length);
  } while(--blocks);

  channel[c].enable = 0;
  if(channel[c].irq.enable) {
    channel[c].irq.flag = 1;
    irq.poll();
  }
}

auto DMA::transferLinked(uint c) -> void {
  if(channel[c].direction == 0) return;  //invalid direction for this mode

  channel[c].trigger = 0;

  u32 address = channel[c].address;
  u32 timeout = 0x40000;  //unverified; prevents potential infinite loop

  do {
    u32 header = bus.readWord(address);

    for(uint count : range(header >> 24)) {
      address += 4;
      u32 data = bus.readWord(address);

      if(c == 0) {
        debug(unimplemented, "DMA MDECin linked");
      }
      if(c == 1) {
        debug(unimplemented, "DMA MDECout linked");
      }
      if(c == 2) {
        gpu.writeGP0(data);
      }
      if(c == 3) {
        debug(unimplemented, "DMA CDROM linked");
      }
      if(c == 4) {
        debug(unimplemented, "DMA SPU linked");
      }
      if(c == 5) {
        debug(unimplemented, "DMA PIO linked");
      }
      if(c == 6) {
        debug(unimplemented, "DMA OTC linked");
      }
    }

    address = header & 0xffffff;
    if(address & 0x800000) break;
  } while(--timeout);

  channel[c].enable = 0;
  if(channel[c].irq.enable) {
    channel[c].irq.flag = 1;
    irq.poll();
  }
}
