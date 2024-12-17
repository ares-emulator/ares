auto PI::aesCommandFinished() -> void {
  if(!bb_aes.chainIV) aes.setIV(bb_nand.buffer, bb_aes.ivOffset * AES::AES_BLOCK_SIZE);
  aes.decodeCBC(bb_nand.buffer, bb_aes.bufferOffset * AES::AES_BLOCK_SIZE, bb_aes.dataSize + 1);
  bb_aes.busy = 0;
  if (bb_aes.intrDone)
    mi.raise(MI::IRQ::AES);
}

auto PI::nandCommandFinished() -> void {
  NAND *nand = bb_nand.nand[bb_nand.io.deviceSel];

  switch (bb_nand.io.command) {
    case NAND::Command::Read0: { // read 1 (offset 0)
      nand->setPageOffset(0x000);
      n2 result = nand->read(bb_nand.buffer, bb_nand.io.bufferSel, bb_nand.io.pageNumber, bb_nand.io.xferLen, bb_nand.io.ecc);
      bb_nand.io.dbErr = result.bit(0);
      bb_nand.io.sbErr = result.bit(1);
    } break;

    case NAND::Command::Read1: { // read 1 (offset 256)
      nand->setPageOffset(0x100);
      n2 result = nand->read(bb_nand.buffer, bb_nand.io.bufferSel, bb_nand.io.pageNumber, bb_nand.io.xferLen, bb_nand.io.ecc);
      bb_nand.io.dbErr = result.bit(0);
      bb_nand.io.sbErr = result.bit(1);
    } break;

    case NAND::Command::ReadSpare: {
      nand->setPageOffset(0x200);
      debug(unimplemented, "NAND Command 0x", hex(u8(bb_nand.io.command), 2L), " unimplemented");
    } break;

    case NAND::Command::ReadID: {
      nand->readId(bb_nand.buffer, bb_nand.io.bufferSel, bb_nand.io.xferLen);
    } break;

    case NAND::Command::Reset: {
      nand->power(true);
    } break;

    case NAND::Command::PageProgramC1: {
      nand->writeToBuffer(bb_nand.buffer, bb_nand.io.bufferSel, bb_nand.io.pageNumber, bb_nand.io.xferLen);
    } break;

    case NAND::Command::PageProgramC2: {
      nand->commitWriteBuffer(bb_nand.io.pageNumber);
    } break;

    case NAND::Command::PageProgramDummyC2: {
      nand->queueWriteBuffer(bb_nand.io.pageNumber);
    } break;

    case NAND::Command::CopyBackProgramC2: {
      debug(unimplemented, "NAND Command 0x", hex(u8(bb_nand.io.command), 2L), " unimplemented");
    } break;

    case NAND::Command::CopyBackProgramDummyC1: {
      debug(unimplemented, "NAND Command 0x", hex(u8(bb_nand.io.command), 2L), " unimplemented");
    } break;

    case NAND::Command::BlockEraseC1: {
      nand->queueErasure(bb_nand.io.pageNumber);
    } break;

    case NAND::Command::BlockEraseC2: {
      nand->execErasure();
    } break;

    case NAND::Command::ReadStatus: {
      nand->readStatus(bb_nand.buffer, bb_nand.io.bufferSel, bb_nand.io.xferLen, false);
    } break;

    case NAND::Command::ReadStatusMultiplane: {
      nand->readStatus(bb_nand.buffer, bb_nand.io.bufferSel, bb_nand.io.xferLen, true);
    } break;

    default: {
      debug(unimplemented, "NAND Command 0x", hex(u8(bb_nand.io.command), 2L), " unimplemented");
    } break;
  }
  bb_nand.io.busy = 0;
  if (bb_nand.io.intrDone)
    mi.raise(MI::IRQ::FLASH);
}

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

  if(address == 15) {
    debug(unimplemented, "[PI::regsRead] Read from reg 15");
  }

  //iQue Player

  if(address == 16) {
    //BB_ATB_UPPER
    debug(unusual, "[PI::regsRead] Read from BB_ATB_UPPER");
  }

  if(address == 17) {
    debug(unimplemented, "[PI::regsRead] Read from reg 17");
  }

  if(address == 18) {
    //BB_NAND_CTRL
    if(access().flash) {
      data.bit(31) = bb_nand.io.busy;
      data.bit(30) = bb_nand.io.intrDone;
      data.bit(24,29) = bb_nand.io.unk24_29;
      data.bit(16,23) = u8(bb_nand.io.command);
      data.bit(15) = bb_nand.io.unk15;
      data.bit(14) = bb_nand.io.bufferSel;
      data.bit(12,13) = bb_nand.io.deviceSel;
      data.bit(11) = bb_nand.io.sbErr;
      data.bit(10) = bb_nand.io.dbErr;
      data.bit(0,9) = bb_nand.io.xferLen;
    }
  }

  if(address == 19) {
    //BB_NAND_CFG
    debug(unusual, "[PI::regsRead] Read from BB_NAND_CFG");
  }

  if(address == 20) {
    if(access().aes) {
      data.bit(31) = bb_aes.busy;
      data.bit(30) = bb_aes.intrPending;
      data.bit(16,21) = bb_aes.dataSize;
      data.bit(9,15) = bb_aes.bufferOffset;
      data.bit(1,7) = bb_aes.ivOffset;
      data.bit(0) = bb_aes.chainIV;
    }
  }

  if(address == 21) {
    //PI_ALLOWED_IO
    data.bit(0) = bb_allowed.buf;
    data.bit(1) = bb_allowed.flash;
    data.bit(2) = bb_allowed.atb;
    data.bit(3) = bb_allowed.aes;
    data.bit(4) = bb_allowed.dma;
    data.bit(5) = bb_allowed.gpio;
    data.bit(6) = bb_allowed.ide;
    data.bit(7) = bb_allowed.err;
  }

  if(address == 22) {
    //BB_RD_LEN
    if(access().dma)
      data.bit(0,8) = io.readLength;
  }

  if(address == 23) {
    //BB_WR_LEN
    if(access().dma)
      data.bit(0,8) = io.writeLength;
  }



  if(address == 24) {
    //PI_BB_GPIO
    if(access().gpio) {
      data.bit(0) = (bb_gpio.power.outputEnable) ? bb_gpio.power.lineOut : bb_gpio.power.lineIn;
      data.bit(1) = (bb_gpio.led.outputEnable) ? bb_gpio.led.lineOut : bb_gpio.led.lineIn;
      data.bit(2) = (bb_gpio.rtc_clock.outputEnable) ? bb_gpio.rtc_clock.lineOut : bb_gpio.rtc_clock.lineIn;
      data.bit(3) = (bb_gpio.rtc_data.outputEnable) ? bb_gpio.rtc_data.lineOut : bb_gpio.rtc_data.lineIn;
      data.bit(4) = bb_gpio.power.outputEnable;
      data.bit(5) = bb_gpio.led.outputEnable;
      data.bit(6) = bb_gpio.rtc_clock.outputEnable;
      data.bit(7) = bb_gpio.rtc_data.outputEnable;
      
      data.bit(22,24) = box_id.unk;
      data.bit(25,26) = box_id.clock;
      data.bit(30,31) = box_id.model;
    }
  }

  if(address == 25) {
    debug(unimplemented, "[PI::regsRead] Read from reg 25");
  }

  if(address == 26) {
    debug(unimplemented, "[PI::regsRead] Read from reg 26");
  }

  if(address == 27) {
    debug(unimplemented, "[PI::regsRead] Read from reg 27");
  }

  if(address == 28) {
    if(access().flash)
      data.bit(0,26) = bb_nand.io.pageNumber;
  }

  if(address == 29) {
    debug(unimplemented, "[PI::regsRead] Read from reg 29");
  }

  if(address == 30) {
    debug(unimplemented, "[PI::regsRead] Read from reg 30");
  }

  if(address == 31) {
    debug(unimplemented, "[PI::regsRead] Read from reg 31");
  }

  debugger.io(Read, address, data);
  return data;
}

auto PI::bufRead(u32 address) -> u32 {
  const char *buffer;
  u32 data = 0;

  address &= 0x7ff;

  if(address < 0x4e0) {
    data = bb_nand.buffer.read<Word>(address);

    if(address < 0x200)
      buffer = "NAND Buffer0";
    else if(address < 0x400)
      buffer = "NAND Buffer1";
    else if(address < 0x410)
      buffer = "NAND Spare0";
    else if(address < 0x420)
      buffer = "NAND Spare1";
    else if(address < 0x4d0)
      buffer = "AES Expanded Key";
    else if(address < 0x4e0)
      buffer = "AES IV";
  }
  else
  {
    buffer = "<None>";
  }

  debugger.ioBuffers(Read, address, data, buffer);
  return data;
}

auto PI::atbRead(u32 address) -> u32 {
  auto data = 0;
  debug(unimplemented, "[PI::atbRead] Read from ATB entries");
  debugger.io(Read, address, data);
  return data;
}

auto PI::ideRead(u32 address_) -> u32 {
  if(!access().ide) return 0;

  flushIDE();

  n32 address = address_;

  n32 data;

  auto device = address.bit(17,19);
  switch(device) {
    case 1: {
      data.bit(0,31) = 0;
    } break;
    case 4: {
      data.bit(16,31) = bb_ide[0].data;
    } break;
    case 5: {
      data.bit(16,31) = bb_ide[1].data;
    } break;
    case 6: {
      data.bit(16,31) = bb_ide[2].data;
    } break;
    case 7: {
      data.bit(16,31) = bb_ide[3].data;
    } break;
  }

  debugger.io(Read, address, data);
  return data;
}

auto PI::ioRead(u32 address) -> u32 {
  address &= 0x1f'ffff;
  if(address <= 0x0'ffff) return regsRead(address);
  if(address <= 0x1'04ff) return bufRead(address);
  if(address <= 0x1'07ff) return atbRead(address);
  if(address <= 0x1'ffff) return 0; //unmapped
  return ideRead(address);
}

auto PI::regsWrite(u32 address, u32 data_) -> void {
  if(system._BB()) address = (address & 0x7f) >> 2;
  else             address = (address & 0x3f) >> 2;
  n32 data = data_;

  debugger.io(Write, address, data);

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

  if(address == 13) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 13");
  }

  if(address == 14) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 14");
  }

  if(address == 15) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 15");
  }

  if(address == 16) {
    bb_atb.upper.ivSource = data.bit(8);
    bb_atb.upper.dmaEnable = data.bit(5);
    bb_atb.upper.cpuEnable = data.bit(4);
    bb_atb.upper.maxOffset = 0x4000 * (1 << data.bit(0,3));
  }

  if(address == 17) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 17");
  }

  if(address == 18) {
    if(access().flash) {
      if (bb_nand.io.busy) {
        debug(unusual, "[PI::regsWrite] Write to NAND command register while busy");
      } else {
        bb_nand.io.intrDone = data.bit(30);
        bb_nand.io.unk24_29 = data.bit(24,29);
        bb_nand.io.command = (NAND::Command)u8(data.bit(16,23));
        bb_nand.io.unk15 = data.bit(15);
        bb_nand.io.bufferSel = data.bit(14);
        bb_nand.io.deviceSel = data.bit(12,13);
        bb_nand.io.ecc = data.bit(11);
        bb_nand.io.multiCycle = data.bit(10);
        bb_nand.io.xferLen = data.bit(0,9);

        if (data.bit(31)) { //Execute command
          bb_nand.io.busy = 1;
          queue.insert(Queue::NAND_Command, 200); //TODO cycles, depending on command type
        } else {
          // TODO: which bit actually does this?
          mi.lower(MI::IRQ::FLASH);
        }
      }
    }
  }

  if(address == 19) {
    if(access().flash)
      bb_nand.io.config = data.bit(0,31);
  }

  if(address == 20) {
    if(access().aes) {
      bb_aes.intrDone = data.bit(30);
      bb_aes.dataSize = data.bit(16,21);
      bb_aes.bufferOffset = data.bit(9,15);
      bb_aes.ivOffset = data.bit(1,7);
      bb_aes.chainIV = data.bit(0);

      if(data.bit(31)) { //Execute
        bb_aes.busy = 1;
        queue.insert(Queue::AES_Command, 200); // TODO cycles, proportional to data size
      } else {
        // TODO: which bit actually does this?
        mi.lower(MI::IRQ::AES);
      }
    }
  }

  if(address == 21) {
    //PI_ALLOWED_IO
    bb_allowed.buf   = data.bit(0) & access().buf;
    bb_allowed.flash = data.bit(1) & access().flash;
    bb_allowed.atb   = data.bit(2) & access().atb;
    bb_allowed.aes   = data.bit(3) & access().aes;
    bb_allowed.dma   = data.bit(4) & access().dma;
    bb_allowed.gpio  = data.bit(5) & access().gpio;
    bb_allowed.ide   = data.bit(6) & access().ide;
    bb_allowed.err   = data.bit(7) & access().err;
  }

  if(address == 22) {
    if(access().dma) {
      io.readLength = n24(data + 1);
      io.dmaBusy = 1;
      io.originPc = cpu.ipu.pc;
      queue.insert(Queue::PI_DMA_Read, 200); // TODO cycles
      bufferDMARead();
    }
  }

  if(address == 23) {
    if(access().dma) {
      io.writeLength = n24(data + 1);
      io.dmaBusy = 1;
      io.originPc = cpu.ipu.pc;
      queue.insert(Queue::PI_DMA_Write, 200); // TODO cycles
      bufferDMAWrite();
    }
  }

  if(address == 24) {
    //PI_BB_GPIO
    if(access().gpio) {
      bb_gpio.power.lineOut = data.bit(0);
      bb_gpio.led.lineOut = data.bit(1);
      bb_gpio.rtc_clock.lineOut = data.bit(2);
      bb_gpio.rtc_data.lineOut = data.bit(3);
      bb_gpio.power.outputEnable = data.bit(4);
      bb_gpio.led.outputEnable = data.bit(5);
      bb_gpio.rtc_clock.outputEnable = data.bit(6);
      bb_gpio.rtc_data.outputEnable = data.bit(7);

      bb_rtc.tick();
    }
  }

  if(address == 25) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 25");
  }

  if(address == 26) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 26");
  }

  if(address == 27) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 27");
  }

  if(address == 28) {
    if(access().flash)
      bb_nand.io.pageNumber = data.bit(0,26);
  }

  if(address == 29) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 29");
  }

  if(address == 30) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 30");
  }

  if(address == 31) {
    debug(unimplemented, "[PI::regsWrite] Write to reg 31");
  }

}

auto PI::bufWrite(u32 address, u32 data_) -> void {
  const char *buffer;
  n32 data = data_;

  address &= 0x7ff;

  if(address < 0x4e0) {
    bb_nand.buffer.write<Word>(address, data);

    if(address < 0x200)
      buffer = "NAND Buffer0";
    else if(address < 0x400)
      buffer = "NAND Buffer1";
    else if(address < 0x410)
      buffer = "NAND Spare0";
    else if(address < 0x420)
      buffer = "NAND Spare1";
    else if(address < 0x4d0) {
      buffer = "AES Expanded Key";
      if (bb_aes.busy)
        debug(unusual, "AES key memory changed while AES is busy");
      aes.setKey(bb_nand.buffer);
    }
    else if(address < 0x4e0)
      buffer = "AES IV";
  }
  else
  {
    buffer = "<None>";
  }

  debugger.ioBuffers(Write, address, data, buffer);
}

auto PI::atbWrite(u32 address, u32 data_) -> void {
  n32 data = data_;
  address = (address - 0x10500) >> 2;

  bb_atb.entries[address].ivSource = bb_atb.upper.ivSource;
  bb_atb.entries[address].dmaEnable = bb_atb.upper.dmaEnable;
  bb_atb.entries[address].cpuEnable = bb_atb.upper.cpuEnable;
  bb_atb.entries[address].maxOffset = bb_atb.upper.maxOffset;
  bb_atb.entries[address].nandAddr = 0x4000 * data.bit(16,31);
  bb_atb.pbusAddresses[address] = data.bit(0,15) << 14;

  bb_atb.addressMasks[address] = bb_atb.upper.maxOffset - 1;
  bb_atb.entryCached = -1u;
}

auto PI::ideWrite(u32 address_, u32 data_) -> void {
  if(!access().ide) return;

  flushIDE();

  n32 address = address_;
  n32 data = data_;

  auto device = address.bit(17,19);
  switch(device) {
    case 1: {} break;
    case 4: {
      bb_ide[0].data = data.bit(16,31);
      bb_ide[0].dirty = 1;
    } break;
    case 5: {
      bb_ide[1].data = data.bit(16,31);
      bb_ide[1].dirty = 1;
    } break;
    case 6: {
      bb_ide[2].data = data.bit(16,31);
      bb_ide[2].dirty = 1;
    } break;
    case 7: {
      bb_ide[3].data = data.bit(16,31);
      bb_ide[3].dirty = 1;
    } break;
  }
}

auto PI::ioWrite(u32 address, u32 data) -> void {
  address &= 0x1f'ffff;
  if(address <= 0x0'ffff) return regsWrite(address, data);
  if(address <= 0x1'04ff) return bufWrite(address, data);
  if(address <= 0x1'07ff) return atbWrite(address, data);
  if(address <= 0x1'ffff) return; //unmapped
  return ideWrite(address, data);
}

auto PI::flushIDE() -> void {
  for(n2 which : range(4)) {
    if(bb_ide[which].dirty) {
      debugger.ide(Write, which, bb_ide[which].data);
      bb_ide[which].dirty = 0;
    }
  }
}
