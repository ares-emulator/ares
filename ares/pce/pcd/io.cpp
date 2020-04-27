auto PCD::read(uint8 bank, uint13 address, uint8 data) -> uint8 {
  if(bank >= 0x00 && bank <= 0x3f && Model::PCEngineDuo()) {
    return bios.read(address);
  }

  if(bank >= 0x68 && bank <= 0x78 && Model::PCEngineDuo() && sramEnable()) {
    return sram.read(bank - 0x68 << 13 | address);
  }

  if(bank >= 0x80 && bank <= 0x87) {
    return wram.read(bank - 0x80 << 13 | address);
  }

  if(bank == 0xf7 && bramEnable()) {
    return bram.read(address);
  }

  return data;
}

auto PCD::write(uint8 bank, uint13 address, uint8 data) -> void {
  if(bank >= 0x68 && bank <= 0x7f && Model::PCEngineDuo() && sramEnable()) {
    return sram.write(bank - 0x68 << 13 | address, data);
  }

  if(bank >= 0x80 && bank <= 0x87) {
    return wram.write(bank - 0x80 << 13 | address, data);
  }

  if(bank == 0xf7 && bramEnable()) {
    return bram.write(address, data);
  }
}

auto PCD::readIO(uint13 address, uint8 data) -> uint8 {
  if(address == 0x18c0 && Model::PCEngineDuo()) return 0x00;
  if(address == 0x18c1 && Model::PCEngineDuo()) return 0xaa;
  if(address == 0x18c2 && Model::PCEngineDuo()) return 0x55;
  if(address == 0x18c3 && Model::PCEngineDuo()) return sram.size() / 64_KiB;
  if(address >= 0x18c4) return data;

  scsi.update();
  address = (uint4)address;
  data = io.mdr[address];

  if(address == 0x0) {
    data.bit(3) = scsi.pin.input;
    data.bit(4) = scsi.pin.control;
    data.bit(5) = scsi.pin.message;
    data.bit(6) = scsi.pin.request;
    data.bit(7) = scsi.pin.busy;
    scsi.irq.ready.lower();
    scsi.irq.completed.lower();
  }

  if(address == 0x1) {
    data = scsi.bus.data;
  }

  if(address == 0x2) {
    data.bit(2) = adpcm.irq.halfReached.enable;
    data.bit(3) = adpcm.irq.endReached.enable;
    data.bit(4) = 0;  //???
    data.bit(5) = scsi.irq.completed.enable;
    data.bit(6) = scsi.irq.ready.enable;
    data.bit(7) = scsi.pin.acknowledge;
  }

  if(address == 0x3) {
    data.bit(1) = io.cddaSampleSelect;
    data.bit(2) = adpcm.irq.halfReached.line;
    data.bit(3) = adpcm.irq.endReached.line;
    data.bit(4) = 0;  //???
    data.bit(5) = scsi.irq.completed.line;
    data.bit(6) = scsi.irq.ready.line;
    data.bit(7) = 0;  //???
    io.bramEnable = 0;
    io.cddaSampleSelect = !io.cddaSampleSelect;
  }

  if(address == 0x4) {
    data.bit(1) = scsi.pin.reset;
  }

  if(address == 0x5) {
    auto sample = !io.cddaSampleSelect ? cdda.sample.left : cdda.sample.right;
    data = sample.byte(0);
  }

  if(address == 0x6) {
    auto sample = !io.cddaSampleSelect ? cdda.sample.left : cdda.sample.right;
    data = sample.byte(1);
  }

  if(address == 0x7) {
    data.bit(7) = io.bramEnable;
  }

  if(address == 0x8) {
    data = scsi.readData();
  }

  if(address == 0x9) {
    //write-only
  }

  if(address == 0xa) {
    data = adpcm.read.data;
    adpcm.read.pending = ADPCM::ReadLatency;
  }

  if(address == 0xb) {
    data.bit(0,1) = adpcm.dmaActive;
  }

  if(address == 0xc) {
    data.bit(0) = adpcm.irq.endReached.line;
    data.bit(2) = adpcm.write.pending > 0;
    data.bit(3) = adpcm.io.playing;
    data.bit(7) = adpcm.read.pending > 0;
  }

  if(address == 0xd) {
    data.bit(0) = adpcm.io.writeOffset;
    data.bit(1) = adpcm.io.writeLatch;
    data.bit(2) = adpcm.io.readOffset;
    data.bit(3) = adpcm.io.readLatch;
    data.bit(4) = adpcm.io.lengthLatch;
    data.bit(5) = adpcm.io.playing;
    data.bit(6) = adpcm.io.noRepeat;
    data.bit(7) = adpcm.io.reset;
  }

  if(address == 0xe) {
    //write-only
  }

  if(address == 0xf) {
    //write-only
  }

//if(address != 0x0)
//print("* rd ff:180", hex(address, 1L), " = ", hex(data, 2L), " ", binary(data, 8L), "\n");

  return data;
}

auto PCD::writeIO(uint13 address, uint8 data) -> void {
  if(address == 0x18c0) io.sramEnable = io.sramEnable << 8 | data;
  if(address >= 0x18c4) return;

  address = (uint4)address;

//print("* wr ff:180", hex(address, 1L), " = ", hex(data, 2L), "\n");

  if(address == 0x0) {
    scsi.pin.select = 1;
    scsi.update();
    scsi.pin.select = 0;
    scsi.update();
  }

  if(address == 0x1) {
    scsi.bus.data = data;
    scsi.update();
  }

  if(address == 0x2) {
    adpcm.irq.halfReached.enable = data.bit(2);
    adpcm.irq.endReached.enable  = data.bit(3);
    scsi.irq.completed.enable    = data.bit(5);
    scsi.irq.ready.enable        = data.bit(6);
    scsi.pin.acknowledge         = data.bit(7);
    scsi.update();
  }

  if(address == 0x4) {
    scsi.pin.reset = data.bit(1);
    scsi.update();
  }

  if(address == 0x5) {
    //read-only
  }

  if(address == 0x6) {
    //read-only
  }

  if(address == 0x7) {
    if(data.bit(7)) io.bramEnable = 1;
  }

  if(address == 0x8) {
    adpcm.latch.byte(0) = data;
    if(adpcm.io.lengthLatch) adpcm.length = adpcm.latch;
  }

  if(address == 0x9) {
    adpcm.latch.byte(1) = data;
    if(adpcm.io.lengthLatch) adpcm.length = adpcm.latch;
  }

  if(address == 0xa) {
    adpcm.write.pending = ADPCM::WriteLatency;
    adpcm.write.data = data;
  }

  if(address == 0xb) {
    adpcm.dmaActive = data.bit(0,1);
  }

  if(address == 0xc) {
    //read-only
  }

  if(address == 0xd) {
    adpcm.control(data);
  }

  if(address == 0xe) {
    adpcm.divider = 0x10 - data.bit(0,3);
    adpcm.period  = 0;
  }

  if(address == 0xf && io.mdr[address] != data) {
    if(data.bit(1) == 0) fader.mode = Fader::Mode::CDDA;
    if(data.bit(1) == 1) fader.mode = Fader::Mode::ADPCM;

    if(data.bit(2) == 0) fader.step = 1.0 / 6000;  //6.0 second fade-out
    if(data.bit(2) == 1) fader.step = 1.0 / 2500;  //2.5 second fade-out

    if(!data.bit(3)) {
      fader.mode = Fader::Mode::Idle;
      fader.volume = 1.0;
    }
  }

  io.mdr[address] = data;
}
