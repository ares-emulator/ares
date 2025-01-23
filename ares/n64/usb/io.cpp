auto USB::ioRead(u32 address, Thread& thread) -> u32 {
  address = (address & 0xff) >> 2;
  n32 data = 0;

  if(address == 0) {
    data.bit(0,7) = io.perid;
  }

  if(address == 1) {
    data.bit(0,7) = io.idcomp;
  }

  if(address == 2) {
    data.bit(0,7) = io.rev;
  }

  if(address == 3) {
    data.bit(0,7) = io.addinfo;
  }

  if(address == 4) {
    data.bit(7) = io.otgIntStatus.id;
    data.bit(6) = io.otgIntStatus.oneMSec;
    data.bit(5) = io.otgIntStatus.lineState;
    data.bit(3) = io.otgIntStatus.sessionValid;
    data.bit(2) = io.otgIntStatus.bSession;
    data.bit(0) = io.otgIntStatus.aVBus;
  }

  if(address == 5) {
    data.bit(7) = io.otgIntEnable.id;
    data.bit(6) = io.otgIntEnable.oneMSec;
    data.bit(5) = io.otgIntEnable.lineState;
    data.bit(3) = io.otgIntEnable.sessionValid;
    data.bit(2) = io.otgIntEnable.bSession;
    data.bit(0) = io.otgIntEnable.aVBus;
  }

  if(address == 6) {
    data.bit(7) = io.otgStatus.id;
    data.bit(6) = io.otgStatus.oneMSec;
    data.bit(5) = io.otgStatus.lineStateStable;
    data.bit(3) = io.otgStatus.sessionValid;
    data.bit(2) = io.otgStatus.bSessionEnd;
    data.bit(0) = io.otgStatus.aVbusValid;
  }

  if(address == 7) {
    data.bit(7) = io.otgControl.dpHigh;
    data.bit(5) = io.otgControl.dpLow;
    data.bit(4) = io.otgControl.dmLow;
    data.bit(2) = io.otgControl.otgEnable;
  }

  if(address > 7) {
    debug(unimplemented, "[USB::ioRead] ", address);
  }

  return data;
}

auto USB::ioWrite(u32 address, u32 data_, Thread& thread) -> void {
  address = (address & 0xff) >> 2;
  n32 data = data_;

  if(address == 0) {
    //Read-only
  }

  if(address == 1) {
    //Read-only
  }

  if(address == 2) {
    //Read-only
  }

  if(address == 3) {
    //Read-only
  }

  if(address == 4) {
    //TODO: writing to this register acks interrupts
    io.otgIntStatus.id = data.bit(7);
    io.otgIntStatus.oneMSec = data.bit(6);
    io.otgIntStatus.lineState = data.bit(5);
    io.otgIntStatus.sessionValid = data.bit(3);
    io.otgIntStatus.bSession = data.bit(2);
    io.otgIntStatus.aVBus = data.bit(0);
  }

  if(address == 5) {
    io.otgIntEnable.id = data.bit(7);
    io.otgIntEnable.oneMSec = data.bit(6);
    io.otgIntEnable.lineState = data.bit(5);
    io.otgIntEnable.sessionValid = data.bit(3);
    io.otgIntEnable.bSession = data.bit(2);
    io.otgIntEnable.aVBus = data.bit(0);
  }

  if(address == 6) {
    //Read-only (?)
  }

  if(address == 7) {
    io.otgControl.dpHigh = data.bit(7);
    io.otgControl.dpLow = data.bit(5);
    io.otgControl.dmLow = data.bit(4);
    io.otgControl.otgEnable = data.bit(2);
  }

  if(address > 7) {
    debug(unimplemented, "[USB::ioWrite] ", address, " <= ", hex(data, 8L));
  }
}

auto USB::readWord(u32 address, Thread& thread) -> u32 {
  if(!mi.secure() && !io.access) {
    debug(unusual, "[USB::readWord] Read from USB address space without access");
    return 0;
  }

  address &= 0xfffff;

  if (address < 0x40000) {
    return ioRead(address & 0x3ffff, thread);
  } else if (address < 0x80000) {
    return u32(io.access);
  } else {
    return bdt.read<Word>(address);
  }
}

auto USB::writeWord(u32 address, u32 data, Thread& thread) -> void {
  if(!mi.secure() && !io.access) {
    debug(unusual, "[USB::readWord] Write to USB address space without access");
    return;
  }

  address &= 0xfffff;

  if (address < 0x40000) {
    ioWrite(address & 0x3ffff, data, thread);
  } else if (address < 0x80000) {
    io.access = data;
  } else {
    bdt.write<Word>(address, data);
  }
}
