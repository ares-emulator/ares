auto PCD::read(uint4 address) -> uint8 {
  scsi.update();
  uint8 data = io.mdr[address];

  if(address == 0x0) {
    data.bit(3) = scsi.input;
    data.bit(4) = scsi.control;
    data.bit(5) = scsi.message;
    data.bit(6) = scsi.request;
    data.bit(7) = scsi.busy;
    scsi.irq.ready.pending = 0;
    scsi.irq.completed.pending = 0;
  }

  if(address == 0x1) {
    data = scsi.data;
  }

  if(address == 0x2) {
    data.bit(2) = 0;  //ADPCM half
    data.bit(3) = 0;  //ADPCM full
    data.bit(4) = 0;  //BRAM
    data.bit(5) = scsi.irq.completed.enable;
    data.bit(6) = scsi.irq.ready.enable;
    data.bit(7) = scsi.acknowledge;
  }

  if(address == 0x3) {
    data.bit(2) = 0;  //ADPCM half
    data.bit(3) = 0;  //ADPCM full
    data.bit(4) = 1;
    data.bit(5) = scsi.irq.completed.pending;
    data.bit(6) = scsi.irq.ready.pending;
    data.bit(7) = 0;
    bram.writable = 0;
  }

  if(address == 0x4) {
    data.bit(1) = scsi.reset;
  }

  if(address == 0x7) {
    data.bit(7) = bram.writable;
  }

  if(address == 0x8) {
    data = scsi.readData();
  }

//if(address != 0x0)
//print("* rd ff:180", hex(address, 1L), " = ", hex(data, 2L), " ", binary(data, 8L), "\n");

  return data;
}

auto PCD::write(uint4 address, uint8 data) -> void {
//print("* wr ff:180", hex(address, 1L), " = ", hex(data, 2L), "\n");

  io.mdr[address] = data;

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
  //irq.adpcmHalf.enable      = data.bit(2);
  //irq.adpcmFull.enable      = data.bit(3);
  //irq.bram.enable           = data.bit(4);
    scsi.irq.completed.enable = data.bit(5);
    scsi.irq.ready.enable     = data.bit(6);
    scsi.acknowledge          = data.bit(7);
    scsi.update();
  }

  if(address == 0x4) {
    scsi.reset = data.bit(1);
    scsi.update();
  }

  if(address == 0x7) {
    if(data.bit(7)) bram.writable = 1;
  }
}
