auto DD::readWord(u32 address) -> u32 {
  address = (address & 0x7f) >> 2;
  n32 data;

  //ASIC_DATA
  if(address == 0) {
    data.bit(16,31) = io.data;
  }

  //ASIC_MISC_REG
  if(address == 1) {
  }

  //ASIC_STATUS
  if(address == 2) {
    data.bit(16) = io.status.diskChanged;
    data.bit(17) = io.status.mechaError;
    data.bit(18) = io.status.writeProtect;
    data.bit(19) = io.status.headRetracted;
    data.bit(20) = io.status.spindleMotorStopped;
    data.bit(22) = io.status.resetState;
    data.bit(23) = io.status.busyState;
    data.bit(24) = 0; //disk present
    data.bit(25) = irq.mecha.line;
    data.bit(26) = irq.bm.line;
    data.bit(27) = io.bm.error;
    data.bit(28) = io.status.requestC2Sector;
    data.bit(30) = io.status.requestUserSector;

    //required to indicate the 64DD is missing
    //data = 0xffff'ffff;
  }

  //ASIC_CUR_TK
  if(address == 3) {
    data.bit(16,31) = io.currentTrack | 0x6000;
  }

  //ASIC_BM_STATUS
  if(address == 4) {
    data.bit(16) = io.bm.c1Error;
    data.bit(21) = io.bm.c1Single;
    data.bit(22) = io.bm.c1Double;
    data.bit(23) = io.bm.c1Correct;
    data.bit(24) = io.bm.blockTransfer;
    data.bit(25) = io.micro.error;
    data.bit(26) = io.bm.error;
    data.bit(31) = io.bm.start;
  }

  //ASIC_ERR_SECTOR
  if(address == 5) {
    data.bit(24) = io.error.selfStop;
    data.bit(25) = io.error.clockUnlock;
    data.bit(26) = 0; //no disk
    data.bit(27) = io.error.offTrack;
    data.bit(28) = io.error.overrun;
    data.bit(29) = io.error.spindle;
    data.bit(30) = io.micro.error;
    data.bit(31) = io.error.am;
  }

  //ASIC_SEQ_STATUS
  if(address == 6) {
    data.bit(30) = io.micro.enable;
  }

  //ASIC_CUR_SECTOR
  if(address == 7) {
    data.bit(16,23) = io.currentSector;
  }

  //ASIC_HARD_RESET
  if(address == 8) {
  }

  //ASIC_C1_S0
  if(address == 9) {
  }

  //ASIC_HOST_SECBYTE
  if(address == 10) {
    data.bit(16,23) = io.sectorSizeBuf;
  }

  //ASIC_C1_S2
  if(address == 11) {
  }

  //ASIC_SEC_BYTE
  if(address == 12) {
    data.bit(16,23) = io.sectorSize;
    data.bit(24,31) = io.sectorBlock;
  }

  //ASIC_C1_S4
  if(address == 13) {
  }

  //ASIC_C1_S6
  if(address == 14) {
  }

  //ASIC_CUR_ADDRESS
  if(address == 15) {
  }

  //ASIC_ID_REG
  if(address == 16) {
    data.bit(16,31) = io.id;
  }

  //ASIC_TEST_REG
  if(address == 17) {
  }

  //ASIC_TEST_PIN_SEL
  if(address == 18) {
  }

  debugger.io(Read, address, data);
  return data;
}

auto DD::writeWord(u32 address, u32 data_) -> void {
  address = (address & 0x7f) >> 2;
  n32 data = data_;

  //ASIC_DATA
  if(address == 0) {
    io.data = data.bit(16,31);
  }

  //ASIC_MISC_REG
  if(address == 1) {
  }

  //ASIC_CMD
  if(address == 2) {
    command(data.bit(16,31));
  }

  //ASIC_CUR_TK
  if(address == 3) {
  }

  //ASIC_BM_CTL
  if(address == 4) {
    io.bm.start |= data.bit(31);
    io.bm.readMode = data.bit(30);
    irq.bm.mask = ~data.bit(29);
    if (data.bit(28)) {
      //BM reset
      lower(IRQ::BM);
    }
    io.bm.disableORcheck = data.bit(27);
    io.bm.disableC1Correction = data.bit(26);
    io.bm.blockTransfer = data.bit(25);
    if (data.bit(24)) {
      //Mecha Int Reset
      lower(IRQ::MECHA);
    }
  }

  //ASIC_ERR_SECTOR
  if(address == 5) {
  }

  //ASIC_SEQ_CTL
  if(address == 6) {
  }

  //ASIC_CUR_SECTOR
  if(address == 7) {
  }

  //ASIC_HARD_RESET
  if(address == 8) {
    if((data >> 16) == 0xAAAA) {
      //reset();
    }
  }

  //ASIC_C1_S0
  if(address == 9) {
  }

  //ASIC_HOST_SECBYTE
  if(address == 10) {
    io.sectorSizeBuf = data.bit(16,23);
  }

  //ASIC_C1_S2
  if(address == 11) {
    io.sectorSize = data.bit(16,23);
    io.sectorBlock = data.bit(24,31);
  }

  //ASIC_SEC_BYTE
  if(address == 12) {
  }

  //ASIC_C1_S4
  if(address == 13) {
  }

  //ASIC_C1_S6
  if(address == 14) {
  }

  //ASIC_CUR_ADDRESS
  if(address == 15) {
  }

  //ASIC_ID_REG
  if(address == 16) {
  }

  //ASIC_TEST_REG
  if(address == 17) {
  }

  //ASIC_TEST_PIN_SEL
  if(address == 18) {
  }

  debugger.io(Write, address, data);
}
