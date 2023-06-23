auto DSP::read(n7 address) -> n8 {
  return registers[address];
}

auto DSP::write(n7 address, n8 data) -> void {
  registers[address] = data;

  switch(address) {
  case 0x0c:  //MVOLL
    mainvol.volume[0] = data;
    break;
  case 0x1c:  //MVOLR
    mainvol.volume[1] = data;
    break;
  case 0x2c:  //EVOLL
    echo.volume[0] = data;
    break;
  case 0x3c:  //EVOLR
    echo.volume[1] = data;
    break;
  case 0x4c:  //KON
    for(u32 n : range(8)) voice[n].keyon = data.bit(n);
    for(u32 n : range(8)) voice[n]._keylatch = data.bit(n);
    break;
  case 0x5c:  //KOFF
    for(u32 n : range(8)) voice[n].keyoff = data.bit(n);
    break;
  case 0x6c:  //FLG
    noise.frequency = data.bit(0,4);
    echo.readonly   = data.bit(5);
    mainvol.mute     = data.bit(6);
    mainvol.reset    = data.bit(7);
    break;
  case 0x7c:  //ENDX
    for(u32 n : range(8)) voice[n]._end = 0;
    registers[0x7c] = 0;  //always cleared, regardless of data written
    break;
  case 0x0d:  //EFB
    echo.feedback = data;
    break;
  case 0x2d:  //PMON
    for(u32 n : range(8)) voice[n].modulate = data.bit(n);
    voice[0].modulate = 0;  //voice 0 does not support modulation
    break;
  case 0x3d:  //NON
    for(u32 n : range(8)) voice[n].noise = data.bit(n);
    break;
  case 0x4d:  //EON
    for(u32 n : range(8)) voice[n].echo = data.bit(n);
    break;
  case 0x5d:  //DIR
    brr.bank = data;
    break;
  case 0x6d:  //ESA
    echo.page = data;
    break;
  case 0x7d:  //EDL
    echo.delay = data.bit(0,3);
    break;
  }

  n3 n = address.bit(4,6);
  switch((n4)address) {
  case 0x00:  //VxVOLL
    voice[n].volume[0] = data;
    break;
  case 0x01:  //VxVOLR
    voice[n].volume[1] = data;
    break;
  case 0x02:  //VxPITCHL
    voice[n].pitch.bit(0,7) = data.bit(0,7);
    break;
  case 0x03:  //VxPITCHH
    voice[n].pitch.bit(8,13) = data.bit(0,5);
    break;
  case 0x04:  //VxSRCN
    voice[n].source = data;
    break;
  case 0x05:  //VxADSR0
    voice[n].adsr0 = data;
    break;
  case 0x06:  //VxADSR1
    voice[n].adsr1 = data;
    break;
  case 0x07:  //VxGAIN
    voice[n].gain = data;
    break;
  case 0x08:  //VxENVX
    latch.envx = data;
    break;
  case 0x09:  //VxOUTX
    latch.outx = data;
    break;
  case 0x0f:  //FIRx
    echo.fir[n] = data;
    break;
  }
}
