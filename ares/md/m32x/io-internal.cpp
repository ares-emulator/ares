auto M32X::readInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> n16 {
  //interrupt mask
  if(address == 0x4000) {
    data.bit( 8) = (bool)cartridge.node;  //1 = cartridge connected
    data.bit( 9) = io.adapterEnable;
    data.bit(15) = io.framebufferAccess;
  }

  //68K to SH2 DREQ source address
  if(address == 0x4008) {
    data.byte(0) = dreq.source.byte(2);
  }
  if(address == 0x400a) {
    data.byte(1) = dreq.source.byte(1);
    data.byte(0) = dreq.source.byte(0);
  }

  //68K to SH2 DREQ target address
  if(address == 0x400c) {
    data.byte(0) = dreq.target.byte(2);
  }
  if(address == 0x400e) {
    data.byte(1) = dreq.target.byte(1);
    data.byte(0) = dreq.target.byte(0);
  }

  //68K to SH2 DREQ length
  if(address == 0x4010) {
    data.byte(1) = dreq.length.byte(1);
    data.byte(0) = dreq.length.byte(0);
  }

  //FIFO
  if(address == 0x4012) {
    //todo
  }

  //communication
  if(address >= 0x4020 && address <= 0x402f) {
    data = communication[address >> 1 & 7];
  }

  return data;
}

auto M32X::writeInternalIO(n1 upper, n1 lower, n29 address, n16 data) -> void {
  //interrupt mask
  if(address == 0x4000) {
    //...
  }

  //stand by change
  if(address == 0x4002) {
  }

  //communication
  if(address >= 0x4020 && address <= 0x402f) {
    if(upper) communication[address >> 1 & 7].byte(1) = data.byte(1);
    if(lower) communication[address >> 1 & 7].byte(0) = data.byte(0);
  }
}
