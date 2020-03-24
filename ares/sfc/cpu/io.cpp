auto CPU::readRAM(uint24 address, uint8 data) -> uint8 {
  return wram[address];
}

auto CPU::readAPU(uint24 address, uint8 data) -> uint8 {
  synchronize(smp);
  return smp.portRead(address.bit(0,1));
}

auto CPU::readCPU(uint24 address, uint8 data) -> uint8 {
  switch((uint16)address) {
  case 0x2180:  //WMDATA
    return bus.read(0x7e0000 | io.wramAddress++, data);

  case 0x4016:  //JOYSER0
    data.bit(0,1) = controllerPort1.data();
    return data;

  case 0x4017:  //JOYSER1
    data.bit(0,1) = controllerPort2.data();
    data.bit(2,4) = 0b111;  //these pins are connected to GND
    return data;

  case 0x4210:  //RDNMI
    data.bit(0,3) = io.version;
    data.bit(7)   = rdnmi();
    return data;

  case 0x4211:  //TIMEUP
    data.bit(7) = timeup();
    return data;

  case 0x4212:  //HVBJOY
    data.bit(0) = status.autoJoypadActive;
    data.bit(6) = hcounter() <= 2 || hcounter() >= 1096;  //Hblank
    data.bit(7) = vcounter() >= ppu.vdisp();              //Vblank
    return data;

  case 0x4213: return io.pio;  //RDIO

  case 0x4214: return io.rddiv.byte(0);  //RDDIVL
  case 0x4215: return io.rddiv.byte(1);  //RDDIVH
  case 0x4216: return io.rdmpy.byte(0);  //RDMPYL
  case 0x4217: return io.rdmpy.byte(1);  //RDMPYH

  case 0x4218: return io.joy1.byte(0);   //JOY1L
  case 0x4219: return io.joy1.byte(1);   //JOY1H
  case 0x421a: return io.joy2.byte(0);   //JOY2L
  case 0x421b: return io.joy2.byte(1);   //JOY2H
  case 0x421c: return io.joy3.byte(0);   //JOY3L
  case 0x421d: return io.joy3.byte(1);   //JOY3H
  case 0x421e: return io.joy4.byte(0);   //JOY4L
  case 0x421f: return io.joy4.byte(1);   //JOY4H

  }

  return data;
}

auto CPU::readDMA(uint24 address, uint8 data) -> uint8 {
  auto& channel = this->channels[address.bit(4,6)];

  switch(address & 0xff8f) {

  case 0x4300:  //DMAPx
    data.bit(0,2) = channel.transferMode;
    data.bit(3)   = channel.fixedTransfer;
    data.bit(4)   = channel.reverseTransfer;
    data.bit(5)   = channel.unused;
    data.bit(6)   = channel.indirect;
    data.bit(7)   = channel.direction;
    return data;

  case 0x4301: return channel.targetAddress;          //BBADx
  case 0x4302: return channel.sourceAddress.byte(0);  //A1TxL
  case 0x4303: return channel.sourceAddress.byte(1);  //A1TxH
  case 0x4304: return channel.sourceBank;             //A1Bx
  case 0x4305: return channel.transferSize.byte(0);   //DASxL
  case 0x4306: return channel.transferSize.byte(1);   //DASxH
  case 0x4307: return channel.indirectBank;           //DASBx
  case 0x4308: return channel.hdmaAddress.byte(0);    //A2AxL
  case 0x4309: return channel.hdmaAddress.byte(1);    //A2AxH
  case 0x430a: return channel.lineCounter;            //NTRLx
  case 0x430b: return channel.unknown;                //???x
  case 0x430f: return channel.unknown;                //???x (mirror of $43xb)

  }

  return data;
}

auto CPU::writeRAM(uint24 address, uint8 data) -> void {
  wram[address] = data;
}

auto CPU::writeAPU(uint24 address, uint8 data) -> void {
  synchronize(smp);
  return smp.portWrite(address.bit(0,1), data);
}

auto CPU::writeCPU(uint24 address, uint8 data) -> void {
  switch((uint16)address) {

  case 0x2180:  //WMDATA
    return bus.write(0x7e0000 | io.wramAddress++, data);

  case 0x2181:  //WMADDL
    io.wramAddress.bit(0,7) = data.bit(0,7);
    return;

  case 0x2182:  //WMADDM
    io.wramAddress.bit(8,15) = data.bit(0,7);
    return;

  case 0x2183:  //WMADDH
    io.wramAddress.bit(16) = data.bit(0);
    return;

  case 0x4016:  //JOYSER0
    //bit 0 is shared between JOYSER0 and JOYSER1:
    //strobing $4016.d0 affects both controller port latches.
    //$4017 bit 0 writes are ignored.
    controllerPort1.latch(data.bit(0));
    controllerPort2.latch(data.bit(0));
    return;

  case 0x4200:  //NMITIMEN
    io.autoJoypadPoll = data.bit(0);
    nmitimenUpdate(data);
    return;

  case 0x4201:  //WRIO
    if(io.pio.bit(7) && !data.bit(7)) ppu.latchCounters();
    io.pio = data;
    return;

  case 0x4202:  //WRMPYA
    io.wrmpya = data;
    return;

  case 0x4203:  //WRMPYB
    io.rdmpy = 0;
    if(alu.mpyctr || alu.divctr) return;

    io.wrmpyb = data;
    io.rddiv = io.wrmpyb << 8 | io.wrmpya;

    alu.mpyctr = 8;  //perform multiplication over the next eight cycles
    alu.shift = io.wrmpyb;
    return;

  case 0x4204:  //WRDIVL
    io.wrdiva.byte(0) = data;
    return;

  case 0x4205:  //WRDIVH
    io.wrdiva.byte(1) = data;
    return;

  case 0x4206:  //WRDIVB
    io.rdmpy = io.wrdiva;
    if(alu.mpyctr || alu.divctr) return;

    io.wrdivb = data;

    alu.divctr = 16;  //perform division over the next sixteen cycles
    alu.shift = io.wrdivb << 16;
    return;

  case 0x4207:  //HTIMEL
    io.htime = (io.htime >> 2) - 1;
    io.htime.bit(0,7) = data.bit(0,7);
    io.htime = (io.htime + 1) << 2;
    irqPoll();
    return;

  case 0x4208:  //HTIMEH
    io.htime = (io.htime >> 2) - 1;
    io.htime.bit(8) = data.bit(0);
    io.htime = (io.htime + 1) << 2;
    irqPoll();
    return;

  case 0x4209:  //VTIMEL
    io.vtime.bit(0,7) = data.bit(0,7);
    irqPoll();
    return;

  case 0x420a:  //VTIMEH
    io.vtime.bit(8) = data.bit(0);
    irqPoll();
    return;

  case 0x420b:  //DMAEN
    for(uint n : range(8)) channels[n].dmaEnable = data.bit(n);
    if(data) status.dmaPending = true;
    return;

  case 0x420c:  //HDMAEN
    for(uint n : range(8)) channels[n].hdmaEnable = data.bit(n);
    return;

  case 0x420d:  //MEMSEL
    io.romSpeed = data.bit(0) ? 6 : 8;
    return;

  }
}

auto CPU::writeDMA(uint24 address, uint8 data) -> void {
  auto& channel = this->channels[address.bit(4,6)];

  switch(address & 0xff8f) {

  case 0x4300:  //DMAPx
    channel.transferMode    = data.bit(0,2);
    channel.fixedTransfer   = data.bit(3);
    channel.reverseTransfer = data.bit(4);
    channel.unused          = data.bit(5);
    channel.indirect        = data.bit(6);
    channel.direction       = data.bit(7);
    return;

  case 0x4301:  //BBADx
    channel.targetAddress = data;
    return;

  case 0x4302:  //A1TxL
    channel.sourceAddress.byte(0) = data;
    return;

  case 0x4303:  //A1TxH
    channel.sourceAddress.byte(1) = data;
    return;

  case 0x4304:  //A1Bx
    channel.sourceBank = data;
    return;

  case 0x4305:  //DASxL
    channel.transferSize.byte(0) = data;
    return;

  case 0x4306:  //DASxH
    channel.transferSize.byte(1) = data;
    return;

  case 0x4307:  //DASBx
    channel.indirectBank = data;
    return;

  case 0x4308:  //A2AxL
    channel.hdmaAddress.byte(0) = data;
    return;

  case 0x4309:  //A2AxH
    channel.hdmaAddress.byte(1) = data;
    return;

  case 0x430a:  //NTRLx
    channel.lineCounter = data;
    return;

  case 0x430b:  //???x
    channel.unknown = data;
    return;

  case 0x430f:  //???x (mirror of $43xb)
    channel.unknown = data;
    return;

  }
}
