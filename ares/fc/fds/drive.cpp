auto FDSDrive::clock() -> void {
  if(changing) return change();
  if(!power) return;
  if(!ready) return powerup();
  if(rewinding) return rewind();
  if(scanning) return advance();
}

//presume it takes roughly half a second to eject a disk and insert the other side
auto FDSDrive::change() -> void {
  if(++counter >= 960000) {
    counter = 0;
    changing = 0;
  }
}

//presume that it takes time for the drive motor to power up after being turned on
auto FDSDrive::powerup() -> void {
  if(++counter >= 1600) {
    counter = 0;
    ready = 1;
    rewinding = 1;
    scanning = 0;
  }
}

//presume it takes time to move the drive head back to the beginning of the disk
//the further into the disk the head is, the longer it should take to reach the beginning again
auto FDSDrive::rewind() -> void {
  if(offset && ++counter >= 16) {
    counter = 0;
    offset--;
  }
  if(offset == 0) {
    rewinding = 0;
    completed = 0;
    scanning = scan;
  }
}

//move the disk head periodically
auto FDSDrive::advance() -> void {
  if(++counter >= 160) {
    counter = 0;
    if(reading) {
      read();
    } else {
      write();
    }

    //has the end of the disk been reached?
    if(offset >= fds.inserted->size()) {
      rewinding = 1;
      scanning = 0;
      completed = 1;
    }
  }
}

auto FDSDrive::crc(uint8 data) -> void {
  for(uint index : range(8)) {
    bool carry = crc16.bit(0);
    crc16 = crc16 >> 1 | data.bit(index) << 15;
    if(carry) crc16 ^= 0x8408;
  }
}

//read data from the disk
auto FDSDrive::read() -> void {
  data = fds.inserted->read(offset++);
  if(clearCRC) return (void)(gap = 1);
  if(gap) {
    gap = !data.bit(7);
  } else {
    pending = 1;
    fds.poll();
  }
  if(!gap) crc(data);
}

//write data to the disk
auto FDSDrive::write() -> void {
  if(clearCRC) {
    //in write mode, this causes a stream of 0x00s to be written to the disk instead
    fds.inserted->write(offset++, 0x00);
    crc16 = 0;
    return;
  }

  if(writeCRC) {
    fds.inserted->write(offset++, crc16);
    crc16 >>= 8;
  } else {
    fds.inserted->write(offset++, data);
    crc(data);
  }

  available = 1;
  pending = 1;
  fds.poll();
}

auto FDSDrive::read(uint16 address, uint8 data) -> uint8 {
  switch(address) {

  case 0x4030:
    data.bit(1) = pending;
    data.bit(4) = (bool)crc16;  //invalidCRC;
    data.bit(6) = completed;
    data.bit(7) = available;
    pending = 0;
    fds.poll();
    return data;

  case 0x4031:
    data = this->data;
    pending = 0;
    available = 0;
    fds.poll();
    return data;

  case 0x4032:
    data.bit(0) = !fds.inserted || changing;  //0 = disk inserted, 1 = disk not inserted
    data.bit(1) = !scan || !scanning;         //0 = disk ready, 1 = disk not ready
    data.bit(2) = !fds.inserted || changing;  //0 = not write protected, 1 = write protected
    data.bit(6) = 1;                          //unknown purpose
    return data;

  case 0x4033:
    data.bit(7) = 1;  //0 = battery voltage low, 1 = battery good
    return data;

  }

  return data;
}

auto FDSDrive::write(uint16 address, uint8 data) -> void {
  switch(address) {

  case 0x4023:
    enable = data.bit(0);
    return;

  case 0x4024:
    pending = 0;
    fds.poll();
    if(!enable) return;

    this->data = data;
    available = 0;
    return;

  case 0x4025:
    pending = 0;
    fds.poll();
    if(!enable) return;

    if(!power && data.bit(0)) ready = 0;
    if(!writeCRC && data.bit(4) && !data.bit(2)) crc(0x00), crc(0x00);

    power    = data.bit(0);
    scan     =!data.bit(1);
    reading  = data.bit(2);
    writeCRC = data.bit(4);
    clearCRC =!data.bit(6);
    irq      = data.bit(7);

    if(!power) rewinding = 0;
    if(clearCRC) crc16 = 0;
    return;

  }
}
