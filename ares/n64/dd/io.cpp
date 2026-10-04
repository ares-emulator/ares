auto DD::piAddress(u32 address, PIDeviceTiming timing) -> bool {
  if(!timing.fasterThan(min)) return false;
  asicAccess = false;
  if(address < 0x0500'0000) return false;

  if(address <= 0x0500'03ff) {
    piView = {c2s.data, c2s.size};
    piViewOffset = address & 0x3ff;
    piViewWritable = true;
    return true;
  }
  if(address <= 0x0500'04ff) {
    piView = {ds.data, ds.size};
    piViewOffset = address & 0xff;
    piViewWritable = true;
    return true;
  }
  if(address <= 0x0500'057f) {
    asicAccess = true;
    piViewOffset = address;
    return true;
  }
  if(address <= 0x0500'05bf) {
    piView = {ms.data, ms.size};
    piViewOffset = address & 0x3f;
    piViewWritable = true;
    return true;
  }
  if(address <= 0x05ff'ffff) return false;
  if(address <= 0x063f'ffff) {
    piView = {iplrom.data, iplrom.size};
    piViewOffset = address - 0x0600'0000;
    piViewWritable = false;
    return true;
  }
  return false;
}

auto DD::piReadHalf(PIDeviceTiming timing) -> maybe<u16> {
  if(asicAccess) {
    u16 data = readAsicHalf(piViewOffset);
    debugger.io(Read, (piViewOffset & 0x7f) >> 2, data);
    piViewOffset += 2;
    return data;
  }
  return PIDeviceMemory::piReadHalf(timing);
}

auto DD::piWriteHalf(u16 data, PIDeviceTiming timing) -> void {
  if(asicAccess) {
    writeAsicHalf(piViewOffset, data);
    debugger.io(Write, (piViewOffset & 0x7f) >> 2, data);
    piViewOffset += 2;
    return;
  }
  return PIDeviceMemory::piWriteHalf(data, timing);
}

auto DD::readAsicHalf(u32 address) -> u16 {
  address = (address & 0x7f) >> 1;
  n16 data = 0;

  //ASIC_DATA
  if(address == 0) {
    data.bit(0,15) = io.data;
  }

  //ASIC_MISC_REG
  if(address == 2) {
  }

  //ASIC_STATUS
  if(address == 4) {
    data.bit(0) = io.status.diskChanged;
    data.bit(1) = io.status.mechaError;
    data.bit(2) = io.status.writeProtect;
    data.bit(3) = io.status.headRetracted;
    data.bit(4) = io.status.spindleMotorStopped;
    data.bit(6) = io.status.resetState;
    data.bit(7) = io.status.busyState;
    data.bit(8) = io.status.diskPresent;
    data.bit(9) = irq.mecha.line;
    data.bit(10) = irq.bm.line;
    data.bit(11) = io.bm.error;
    data.bit(12) = io.status.requestC2Sector;
    data.bit(14) = io.status.requestUserSector;

    //acknowledge bm interrupt (tested on real hardware)
    if(irq.bm.line) {
      //TODO: proper research into seek and access times
      cpu.queueInsert(Queue::DD_BM_Request, 38'000 + (io.currentTrack.bit(0,11) / 15));
      lower(IRQ::BM);
    }
  }

  //ASIC_CUR_TK
  if(address == 6) {
    data.bit(0,15) = io.currentTrack;
  }

  //ASIC_BM_STATUS
  if(address == 8) {
    data.bit(0) = io.bm.c1Error;
    data.bit(5) = io.bm.c1Single;
    data.bit(6) = io.bm.c1Double;
    data.bit(7) = io.bm.c1Correct;
    data.bit(8) = io.bm.blockTransfer;
    data.bit(9) = io.micro.error;
    data.bit(10) = io.bm.error;
    data.bit(15) = io.bm.start;
  }

  //ASIC_ERR_SECTOR
  if(address == 10) {
    data.bit(0,7) = io.error.sector;
    data.bit(8) = io.error.selfStop;
    data.bit(9) = io.error.clockUnlock;
    data.bit(10) = ~io.status.diskPresent; //no disk
    data.bit(11) = io.error.offTrack;
    data.bit(12) = io.error.overrun;
    data.bit(13) = io.error.spindle;
    data.bit(14) = io.micro.error;
    data.bit(15) = io.error.am;
  }

  //ASIC_SEQ_STATUS
  if(address == 12) {
  }

  //ASIC_CUR_SECTOR
  if(address == 14) {
    data.bit(8,15) = io.currentSector;
    data.bit(0,7) = 0xc3;
  }

  //ASIC_HARD_RESET
  if(address == 16) {
  }

  //ASIC_C1_S0
  if(address == 18) {
  }

  //ASIC_HOST_SECBYTE
  if(address == 20) {
    data.bit(0,7) = io.sectorSizeBuf;
  }

  //ASIC_C1_S2
  if(address == 22) {
  }

  //ASIC_SEC_BYTE
  if(address == 24) {
    data.bit(0,7) = io.sectorSize;
    data.bit(8,15) = io.sectorBlock;
  }

  //ASIC_C1_S4
  if(address == 26) {
  }

  //ASIC_C1_S6
  if(address == 28) {
  }

  //ASIC_CUR_ADDRESS
  if(address == 30) {
  }

  //ASIC_ID_REG
  if(address == 32) {
    data.bit(0,15) = io.id;
  }

  //ASIC_TEST_REG
  if(address == 34) {
  }

  //ASIC_TEST_PIN_SEL
  if(address == 36) {
  }

  return data;
}

auto DD::writeAsicHalf(u32 address, u16 data_) -> void {
  address = (address & 0x7f) >> 1;
  n16 data = data_;

  //ASIC_DATA
  if(address == 0) {
    io.data = data.bit(0,15);
  }

  //ASIC_MISC_REG
  if(address == 2) {
  }

  //ASIC_CMD
  if(address == 4) {
    command(data.bit(0,15));
  }

  //ASIC_CUR_TK
  if(address == 6) {
  }

  //ASIC_BM_CTL
  if(address == 8) {
    io.bm.reset |= data.bit(12);
    io.bm.readMode = data.bit(14);
    //irq.bm.mask = ~data.bit(13);
    io.bm.disableORcheck = data.bit(11);
    io.bm.disableC1Correction = data.bit(10);
    io.bm.blockTransfer = data.bit(9);
    if (data.bit(8)) {
      //mecha int reset
      lower(IRQ::MECHA);
    }
    io.currentSector = data.bit(0,7);
    if (!data.bit(12) && io.bm.reset) {
      //BM reset
      io.bm.start = 0;
      io.bm.error = 0;
      io.status.requestUserSector = 0;
      io.status.requestC2Sector = 0;
      io.bm.reset = 0;
      lower(IRQ::BM);
    }

    if(data.bit(15) && disk) {
      //start BM
      io.bm.start |= data.bit(15);
      //TODO: proper research into seek and access times
      cpu.queueInsert(Queue::DD_BM_Request, 50'000 + (io.currentTrack.bit(0,11) / 15));
    }
  }

  //ASIC_ERR_SECTOR
  if(address == 10) {
  }

  //ASIC_SEQ_CTL
  if(address == 12) {
    io.micro.enable = data.bit(14);
  }

  //ASIC_CUR_SECTOR
  if(address == 14) {
  }

  //ASIC_HARD_RESET
  if(address == 16) {
    if(data == 0xAAAA) {
      power(true);
    }
  }

  //ASIC_C1_S0
  if(address == 18) {
  }

  //ASIC_HOST_SECBYTE
  if(address == 20) {
    io.sectorSizeBuf = data.bit(0,7);
  }

  //ASIC_C1_S2
  if(address == 22) {
    io.sectorSize = data.bit(0,7);
    io.sectorBlock = data.bit(8,15);
  }

  //ASIC_SEC_BYTE
  if(address == 24) {
  }

  //ASIC_C1_S4
  if(address == 26) {
  }

  //ASIC_C1_S6
  if(address == 28) {
  }

  //ASIC_CUR_ADDRESS
  if(address == 30) {
  }

  //ASIC_ID_REG
  if(address == 32) {
  }

  //ASIC_TEST_REG
  if(address == 34) {
  }

  //ASIC_TEST_PIN_SEL
  if(address == 36) {
  }
}
