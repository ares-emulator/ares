auto PI::regsRead(u32 address) -> u32 {
  if(system._BB()) address = (address & 0x7f) >> 2;
  else             address = (address & 0x3f) >> 2;
  n32 data;

  if(address == 0) {
    //PI_DRAM_ADDRESS
    data = io.dramAddress;
  }

  if(address == 1) {
    //PI_CART_ADDRESS
    data = io.pbusAddress;
  }

  if(address == 2) {
    //PI_READ_LENGTH
    data = io.readLength;
  }

  if(address == 3) {
    //PI_WRITE_LENGTH
    data = io.writeLength;
  }

  if(address == 4) {
    //PI_STATUS
    data.bit(0) = io.dmaBusy;
    data.bit(1) = io.ioBusy;
    data.bit(2) = io.error;
    data.bit(3) = io.interrupt;
  }

  if(address == 5) {
    //PI_BSD_DOM1_LAT
    data.bit(0,7) = bsd1.latency;
  }

  if(address == 6) {
    //PI_BSD_DOM1_PWD
    data.bit(0,7) = bsd1.pulseWidth;
  }

  if(address == 7) {
    //PI_BSD_DOM1_PGS
    data.bit(0,3) = bsd1.pageSize;
  }

  if(address == 8) {
    //PI_BSD_DOM1_RLS
    data.bit(0,1) = bsd1.releaseDuration;
  }

  if(address == 9) {
    //PI_BSD_DOM2_LAT
    data.bit(0,7) = bsd2.latency;
  }

  if(address == 10) {
    //PI_BSD_DOM2_PWD
    data.bit(0,7) = bsd2.pulseWidth;
  }

  if(address == 11) {
    //PI_BSD_DOM2_PGS
    data.bit(0,7) = bsd2.pageSize;
  }

  if(address == 12) {
    //PI_BSD_DOM2_RLS
    data.bit(0,7) = bsd2.releaseDuration;
  }

  if(address == 13) {
    data.bit(0,31) = io.busLatch;
  }

  if(address == 14) {
    data.bit(0,31) = io.busLatch;
  }



  if(address == 24) {
    if(access().gpio) {
      data.bit(0) = bb_gpio.power.data;
      data.bit(1) = bb_gpio.led.data;
      data.bit(2) = bb_gpio.rtc_clock.data;
      data.bit(3) = bb_gpio.rtc_data.data;
      data.bit(4) = bb_gpio.power.mask;
      data.bit(5) = bb_gpio.led.mask;
      data.bit(6) = bb_gpio.rtc_clock.mask;
      data.bit(7) = bb_gpio.rtc_data.mask;
      data.bit(22,24) = box_id.unk;
      data.bit(25,26) = box_id.clock;
      data.bit(30,31) = box_id.model;
    }
  }

  debugger.io(Read, address, data);
  return data;
}

auto PI::bufRead(u32 address) -> u32 {
  auto data = 0;
  debugger.io(Read, address, data);
  return data;
}

auto PI::atbRead(u32 address) -> u32 {
  auto data = 0;
  debugger.io(Read, address, data);
  return data;
}

auto PI::ideRead(u32 address_) -> u32 {
  if(!access().ide) return 0;

  n32 address = address_;

  n32 data;

  auto device = address.bit(17,19);
  switch(device) {
    case 1: {
      data.bit(0,31) = 0;
      if(unlikely(debugger.tracer.io->enabled())) {
        string message = {"IDE3: ", hex(bb_ide[3])};
        debugger.tracer.io->notify(message);
      }
    } break;
    case 4: {
      data.bit(16,31) = bb_ide[0];
    } break;
    case 5: {
      data.bit(16,31) = bb_ide[1];
    } break;
    case 6: {
      data.bit(16,31) = bb_ide[2];
    } break;
    case 7: {
      data.bit(16,31) = bb_ide[3];
    } break;
  }

  debugger.io(Read, address, data);
  return data;
}

auto PI::ioRead(u32 address) -> u32 {
  if(address <= 0x0460'ffff) return regsRead(address);
  if(address <= 0x0461'04ff) return bufRead(address);
  if(address <= 0x0461'07ff) return atbRead(address);
  if(address <= 0x0461'ffff) return 0; //unmapped
  return ideRead(address);
}

auto PI::regsWrite(u32 address, u32 data_) -> void {
  if(system._BB()) address = (address & 0x7f) >> 2;
  else             address = (address & 0x3f) >> 2;
  n32 data = data_;

  //only PI_STATUS can be written while PI is busy
  if(address != 4 && (io.dmaBusy || io.ioBusy)) {
    io.error = 1;
    return;
  }

  if(address == 0) {
    //PI_DRAM_ADDRESS
    io.dramAddress = n24(data) & ~1;
  }

  if(address == 1) {
    //PI_PBUS_ADDRESS
    io.pbusAddress = n32(data) & ~1;
  }

  if(address == 2) {
    //PI_READ_LENGTH
    io.readLength = n24(data);
    io.dmaBusy = 1;
    io.originPc = cpu.ipu.pc;
    queue.insert(Queue::PI_DMA_Read, dmaDuration(true));
    dmaRead();
  }

  if(address == 3) {
    //PI_WRITE_LENGTH
    io.writeLength = n24(data);
    io.dmaBusy = 1;
    io.originPc = cpu.ipu.pc;
    queue.insert(Queue::PI_DMA_Write, dmaDuration(false));
    dmaWrite();
  }

  if(address == 4) {
    //PI_STATUS
    if(data.bit(0)) {
      io.dmaBusy = 0;
      io.error = 0;
      queue.remove(Queue::PI_DMA_Read);
      queue.remove(Queue::PI_DMA_Write);
    }
    if(data.bit(1)) {
      io.interrupt = 0;
      mi.lower(MI::IRQ::PI);
    }
  }

  if(address == 5) {
    //PI_BSD_DOM1_LAT
    bsd1.latency = data.bit(0,7);
  }

  if(address == 6) {
    //PI_BSD_DOM1_PWD
    bsd1.pulseWidth = data.bit(0,7);
  }

  if(address == 7) {
    //PI_BSD_DOM1_PGS
    bsd1.pageSize = data.bit(0,3);
  }

  if(address == 8) {
    //PI_BSD_DOM1_RLS
    bsd1.releaseDuration = data.bit(0,1);
  }

  if(address == 9) {
    //PI_BSD_DOM2_LAT
    bsd2.latency = data.bit(0,7);
  }

  if(address == 10) {
    //PI_BSD_DOM2_PWD
    bsd2.pulseWidth = data.bit(0,7);
  }

  if(address == 11) {
    //PI_BSD_DOM2_PGS
    bsd2.pageSize = data.bit(0,7);
  }

  if(address == 12) {
    //PI_BSD_DOM2_RLS
    bsd2.releaseDuration = data.bit(0,7);
  }



  if(address == 24) {
    if(access().gpio) {
      bb_gpio.power.data = data.bit(0);
      bb_gpio.led.data = data.bit(1);
      bb_gpio.rtc_clock.data = data.bit(2);
      bb_gpio.rtc_data.data = data.bit(3);
      bb_gpio.power.mask = data.bit(4);
      bb_gpio.led.mask = data.bit(5);
      bb_gpio.rtc_clock.mask = data.bit(6);
      bb_gpio.rtc_data.mask = data.bit(7);

      string display = {"[PI::ioWrite] gpio ", string{bb_gpio.led.data}, " ", string{bb_gpio.power.data}};
      debug(unimplemented, display);
    }
  }
}

auto PI::bufWrite(u32 address, u32 data_) -> void {
  return;
}

auto PI::atbWrite(u32 address, u32 data_) -> void {
  return;
}

auto PI::ideWrite(u32 address_, u32 data_) -> void {
  if(!access().ide) return;

  n32 address = address_;
  n32 data = data_;

  auto device = address.bit(17,19);
  switch(device) {
    case 1: {
      if(unlikely(debugger.tracer.io->enabled())) {
        string message = {"IDE3: ", hex(bb_ide[3])};
        debugger.tracer.io->notify(message);
      }
    } break;
    case 4: {
      bb_ide[0] = data.bit(16,31);
    } break;
    case 5: {
      bb_ide[1] = data.bit(16,31);
    } break;
    case 6: {
      bb_ide[2] = data.bit(16,31);
    } break;
    case 7: {
      bb_ide[3] = data.bit(16,31);
    } break;
  }
}

auto PI::ioWrite(u32 address, u32 data) -> void {
  debugger.io(Write, address, data);

  if(address <= 0x0460'ffff) return regsWrite(address, data);
  if(address <= 0x0461'04ff) return bufWrite(address, data);
  if(address <= 0x0461'07ff) return atbWrite(address, data);
  if(address <= 0x0461'ffff) return; //unmapped
  return ideWrite(address, data);
}
