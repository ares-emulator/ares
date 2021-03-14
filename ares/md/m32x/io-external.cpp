auto M32X::readExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  //32X ID
  if(address >= 0xa130ec && address <= 0xa130ef) {
    if(address == 0xa130ec && upper) data.byte(1) = 'M';
    if(address == 0xa130ec && lower) data.byte(0) = 'A';
    if(address == 0xa130ee && upper) data.byte(1) = 'R';
    if(address == 0xa130ee && lower) data.byte(0) = 'S';
    return data;
  }

  //adapter control
  if(address == 0xa15100) {
    data.bit( 0) = io.adapterEnable;
    data.bit( 1) = io.adapterReset;
    data.bit( 7) = io.resetEnable;
    data.bit(15) = io.framebufferAccess;
  }

  //interrupt control
  if(address == 0xa15102) {
    data.bit(0) = io.intm;
    data.bit(1) = io.ints;
  }

  //bank set
  if(address == 0xa15104) {
    data.bit(0) = io.bank0;
    data.bit(1) = io.bank1;
  }

  //data request control
  if(address == 0xa15106) {
  }

  //68K to SH2 DREQ source address
  if(address == 0xa15108) {
    data.byte(0) = dreq.source.byte(2);
  }
  if(address == 0xa1510a) {
    data.byte(1) = dreq.source.byte(1);
    data.byte(0) = dreq.source.byte(0);
  }

  //68K to SH2 DREQ target address
  if(address == 0xa1510c) {
    data.byte(0) = dreq.target.byte(2);
  }
  if(address == 0xa1510e) {
    data.byte(1) = dreq.target.byte(1);
    data.byte(0) = dreq.target.byte(0);
  }

  //68K to SH2 DREQ length
  if(address == 0xa15110) {
    data.byte(1) = dreq.length.byte(1);
    data.byte(0) = dreq.length.byte(0);
  }

  //TV register
  if(address == 0xa1511a) {
    data.bit(0) = io.cartridgeMode;
  }

  //communication
  if(address >= 0xa15120 && address <= 0xa1512f) {
    data = communication[address >> 1 & 7];
  }

  //palette
  if(address >= 0xa15200 && address <= 0xa153ff) {
    return data = palette[address >> 1 & 0xff];
  }

  return data;
}

auto M32X::writeExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> void {
//print("w", hex(address), " = ", hex(data), "\n");

  //adapter control
  if(address == 0xa15100) {
    io.adapterEnable = data.bit(0);
    io.adapterReset  = data.bit(1);
  }

  //interrupt control
  if(address == 0xa15102) {
    io.intm = data.bit(0);
    io.ints = data.bit(1);
  }

  //bank set
  if(address == 0xa15104) {
    io.bank0 = data.bit(0);
    io.bank1 = data.bit(1);
  }

  //data request control
  if(address == 0xa15106) {
  }

  //68K to SH2 DREQ source address
  if(address == 0xa15108) {
    dreq.source.byte(2) = data.byte(0);
  }
  if(address == 0xa1510a) {
    dreq.source.byte(1) = data.byte(1);
    dreq.source.byte(0) = data.byte(0) & ~1;
  }

  //68K to SH2 DREQ target address
  if(address == 0xa1510c) {
    dreq.target.byte(2) = data.byte(0);
  }
  if(address == 0xa1510e) {
    dreq.target.byte(1) = data.byte(1);
    dreq.target.byte(0) = data.byte(0) & ~1;
  }

  //68K to SH2 DREQ length
  if(address == 0xa15110) {
    dreq.length.byte(1) = data.byte(1);
    dreq.length.byte(0) = data.byte(0) & ~3;
  }

  //FIFO
  if(address == 0xa15112) {
    //todo
  }

  //TV register
  if(address == 0xa1511a) {
    io.cartridgeMode = data.bit(0);
  }

  //communication
  if(address >= 0xa15120 && address <= 0xa1512f) {
    if(upper) communication[address >> 1 & 7].byte(1) = data.byte(1);
    if(lower) communication[address >> 1 & 7].byte(0) = data.byte(0);
  }

  //palette
  if(address >= 0xa15200 && address <= 0xa153ff) {
    if(upper) palette[address >> 1 & 0xff].byte(1) = data.byte(1);
    if(lower) palette[address >> 1 & 0xff].byte(0) = data.byte(0);
  }
}
