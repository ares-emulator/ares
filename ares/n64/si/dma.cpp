auto SI::dmaReadBB() -> void {
  // If invalid, return a buffer full of FF
  if(!bbio.valid.bit(0)) {
    for(u32 i : range(32))
      rdram.ram.write<Byte>(io.dramAddress + i, 0xFF, "SI DMA");
    return;
  }

  ControllerPort* controllers[4] = {
    &controllerPort1,
    &controllerPort2,
    &controllerPort3,
    &controllerPort4,
  };

  for(u32 channel : range(4)) {
    // The first byte of every channel is always FF regardless of what it was before
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 0, 0xFF, "SI DMA");

    // The tx length is the input tx length & 0x3F
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 1, 1, "SI DMA");

    // Fill first 4 rx bytes with input data
    n8 output[8];
    output[0].bit(0,7) = bbio.ch[channel].data[1].bit(0,7);
    output[1].bit(0,7) = bbio.ch[channel].data[2].bit(0,7);
    output[2].bit(0,7) = bbio.ch[channel].data[3].bit(0,7);
    output[3].bit(0,7) = bbio.ch[channel].data[4].bit(0,7);

    // Query the controller
    n8 rx;
    rx.bit(0,2) = bbio.ch[channel].rxlen;
    if (controllers[channel]->device) {
      n2 status = controllers[channel]->device->comm(bbio.ch[channel].txlen, bbio.ch[channel].rxlen, bbio.ch[channel].data, output);
      rx.bit(7) = !status.bit(0);
      rx.bit(6)  = status.bit(1);
    } else {
      rx.bit(7) = 1;
      rx.bit(6) = 0;
    }

    // The rx length is the input rx length & 7, with error bits in bits 6 and 7
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 2, u8(rx), "SI DMA");

    // This is the first byte of tx data, always.
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 3, u8(bbio.ch[channel].data[0]), "SI DMA");

    // The last 4 bytes are the first 4 bytes of rx data, if less than 4 bytes of rx data are received the data stays as it was
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 4, u8(output[0]), "SI DMA");
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 5, u8(output[1]), "SI DMA");
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 6, u8(output[2]), "SI DMA");
    rdram.ram.write<Byte>(io.dramAddress + channel * 8 + 7, u8(output[3]), "SI DMA");
  }
}

auto SI::dmaRead() -> void {
  if(system._BB()) {
    dmaReadBB();
  } else {
    pif.dmaRead(io.readAddress, io.dramAddress);
  }
  io.dmaBusy = 0;
  io.pchState = 0;
  io.dmaState = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::SI);
}

auto SI::dmaWriteBB() -> void {
  //If the first byte is not 0xFF it seems to fail to do anything and the buffer
  //returns all 0xFF
  bbio.valid.bit(0) = rdram.ram.read<Byte>(io.dramAddress, "SI DMA") == 0xFF;
  if (!bbio.valid.bit(0)) return;

  for(u32 channel : range(4)) {
    //First byte is generally ignored besides above

    //tx and rx lengths in the next bytes
    u8 txlen = rdram.ram.read<Byte>(io.dramAddress + channel * 8 + 1, "SI DMA") & 0x3F;
    u8 rxlen = rdram.ram.read<Byte>(io.dramAddress + channel * 8 + 2, "SI DMA") & 7;
    bbio.ch[channel].txlen = txlen;
    bbio.ch[channel].rxlen = rxlen;

    //tx data consists of up to 5 bytes of data and then the last byte is repeated for the remainder
    //[FF] [TX] [RX] [__] [__] [__] [__] [__]
    //we copy all 5 bytes so we can echo them back out of rx is less than 4
    u32 i = 0;
    for(; i < 5; i++)
      bbio.ch[channel].data[i].bit(0,7) = rdram.ram.read<Byte>(io.dramAddress + channel * 8 + 3 + i, "SI DMA");
    for(; i < txlen; i++)
      bbio.ch[channel].data[i].bit(0,7) = bbio.ch[channel].data[4].bit(0,7);
  }
}

auto SI::dmaWrite() -> void {
  if(system._BB()) {
    dmaWriteBB();
  } else {
    pif.dmaWrite(io.writeAddress, io.dramAddress);
  }
  io.dmaBusy = 0;
  io.pchState = 0;
  io.dmaState = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::SI);
}
