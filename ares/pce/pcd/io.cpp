auto PCD::read(uint10 address) -> uint8 {
  //hardware identification
//if(address == 0x00c5) return 0xaa;
//if(address == 0x00c6) return 0x55;
//if(address == 0x00c7) return 0x03;
  if(address == 0x00c1) return 0xaa;
  if(address == 0x00c2) return 0x55;
  if(address == 0x00c3) return 0x03;

  scsi.update();
  address = (uint4)address;
  uint8 data = io.mdr[address];

  if(address == 0x0) {
    data.bit(3) = scsi.input;
    data.bit(4) = scsi.control;
    data.bit(5) = scsi.message;
    data.bit(6) = scsi.request;
    data.bit(7) = scsi.busy;
    scsi.irq.ready.pending = 0;
    scsi.irq.completed.pending = 0;
    adpcm.irq.halfPlay.pending = 0;
    adpcm.irq.fullPlay.pending = 0;
  }

  if(address == 0x1) {
    data = scsi.data;
  }

  if(address == 0x2) {
    data.bit(2) = adpcm.irq.halfPlay.enable;
    data.bit(3) = adpcm.irq.fullPlay.enable;
    data.bit(4) = 0;  //???
    data.bit(5) = scsi.irq.completed.enable;
    data.bit(6) = scsi.irq.ready.enable;
    data.bit(7) = scsi.acknowledge;
  }

  if(address == 0x3) {
    data.bit(2) = adpcm.irq.halfPlay.pending;
    data.bit(3) = adpcm.irq.fullPlay.pending;
    data.bit(4) = 0;  //???
    data.bit(5) = scsi.irq.completed.pending;
    data.bit(6) = scsi.irq.ready.pending;
    data.bit(7) = 0;
    bram.writable = 0;
  }

  if(address == 0x4) {
    data.bit(1) = scsi.reset;
  }

  if(address == 0x5) {
    data = 0xff;
  }

  if(address == 0x6) {
    data = 0xff;
  }

  if(address == 0x7) {
    data.bit(7) = bram.writable;
  }

  if(address == 0x8) {
    data = scsi.readData();
  }

  if(address == 0x9) {
    //write-only
  }

  if(address == 0xa) {
    data = adpcm.readRAM();
  }

  if(address == 0xb) {
    //io.mdr[address]
  }

  if(address == 0xc) {
    data.bit(0) = adpcm.stopped;
    data.bit(2) = adpcm.writing;
    data.bit(3) = adpcm.playing;
    data.bit(7) = adpcm.reading;
  }

  if(address == 0xd) {
    data.bit(5) = adpcm.repeat;
    data.bit(6) = adpcm.run;
    data.bit(7) = adpcm.enable;
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
  address = (uint4)address;
  io.mdr[address] = data;

//print("* wr ff:180", hex(address, 1L), " = ", hex(data, 2L), "\n");

  if(address == 0x0) {
    scsi.select = 1;
    scsi.update();
    scsi.select = 0;
    scsi.update();
  }

  if(address == 0x1) {
    scsi.data = data;
    scsi.update();
  }

  if(address == 0x2) {
    adpcm.irq.halfPlay.enable = data.bit(2);
    adpcm.irq.fullPlay.enable = data.bit(3);
    scsi.irq.completed.enable = data.bit(5);
    scsi.irq.ready.enable     = data.bit(6);
    scsi.acknowledge          = data.bit(7);
    scsi.update();
  }

  if(address == 0x4) {
    scsi.reset = data.bit(1);
    scsi.update();
  }

  if(address == 0x5) {
    //read-only
  }

  if(address == 0x6) {
    //read-only
  }

  if(address == 0x7) {
    if(data.bit(7)) bram.writable = 1;
  }

  if(address == 0x8) {
    adpcm.latch.byte(0) = data;
  }

  if(address == 0x9) {
    adpcm.latch.byte(1) = data;
  }

  if(address == 0xa) {
    adpcm.writeRAM(data);
  }

  if(address == 0xb) {
    if(data.bit(0,1)) adpcm.writing = 1;
  }

  if(address == 0xc) {
    //read-only
  }

  if(address == 0xd) {
    adpcm.control(io.mdr[address], data);
  }

  if(address == 0xe) {
    adpcm.divider = 0x10 - data.bit(0,3);
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
