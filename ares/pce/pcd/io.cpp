auto PCD::read(uint10 address) -> uint8 {
  if(address == 0x00c0) return Duo() ? 0x00 : 0xff;
  if(address == 0x00c1) return Duo() ? 0xaa : 0xff;
  if(address == 0x00c2) return Duo() ? 0x55 : 0xff;
  if(address == 0x00c3) return Duo() ? 0x03 : 0xff;

  if(address == 0x00c4) return cartridge.read(0xff, 0x18c4);
  if(address == 0x00c5) return cartridge.read(0xff, 0x18c5);
  if(address == 0x00c6) return cartridge.read(0xff, 0x18c6);
  if(address == 0x00c7) return cartridge.read(0xff, 0x18c7);

  scsi.update();
  address = (uint4)address;
  uint8 data = io.mdr[address];

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
    data.bit(7) = 0;
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
    adpcm.read.pending = 57;
  }

  if(address == 0xb) {
    //io.mdr[address]
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

auto PCD::write(uint10 address, uint8 data) -> void {
  if(address == 0x00c0) io.sramEnable = io.sramEnable << 8 | data;

  address = (uint4)address;
  io.mdr[address] = data;

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
    adpcm.write.pending = 33;
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
    uint4 command = data.bit(0,3);

    if(command == 0x0) {  //CD-DA / ADPCM enable (100ms)
    }

    if(command == 0x1) {  //CD-DA enable (100ms)
    }

    if(command == 0x8) {  //CD-DA short fade-out (1500ms) / ADPCM enable
    }

    if(command == 0x9) {  //CD-DA long fade-out (5000ms)
    }

    if(command == 0xa) {  //ADPCM long fade-out (5000ms)
    }

    if(command == 0xc) {  //CD-DA short fade out (1500ms) / ADPCM enable
    }

    if(command == 0xd) {  //CD-DA short fade out (1500ms)
    }

    if(command == 0xe) {  //ADPCM short fade-out (1500ms)
    }
  }
}
