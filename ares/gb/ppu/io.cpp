auto PPU::vramAddress(uint13 address) const -> uint16 {
  return status.vramBank << 13 | address;
}

auto PPU::readIO(uint cycle, uint16 address, uint8 data) -> uint8 {
  if(address >= 0x8000 && address <= 0x9fff && cycle == 2) {
    if(!canAccessVRAM()) return data;
    return vram[vramAddress(address)];
  }

  if(address >= 0xfe00 && address <= 0xfe9f && cycle == 2) {
    if(!canAccessOAM()) return data;
    return oam[(uint8)address];
  }

  if(address < 0xff40 || address > 0xff7f) return data;

  if(address == 0xff40 && cycle == 2) {  //LCDC
    data.bit(0) = status.bgEnable;
    data.bit(1) = status.obEnable;
    data.bit(2) = status.obSize;
    data.bit(3) = status.bgTilemapSelect;
    data.bit(4) = status.bgTiledataSelect;
    data.bit(5) = status.windowDisplayEnable;
    data.bit(6) = status.windowTilemapSelect;
    data.bit(7) = status.displayEnable;
    return data;
  }

  if(address == 0xff41 && cycle == 2) {  //STAT
    data.bit(0) = history.mode.bit(8);  //status.mode as of 4-cycles ago
    data.bit(1) = history.mode.bit(9);
    data.bit(2) = compareLYC();
    data.bit(3) = status.interruptHblank;
    data.bit(4) = status.interruptVblank;
    data.bit(5) = status.interruptOAM;
    data.bit(6) = status.interruptLYC;
    data.bit(7) = 1;
    return data;
  }

  if(address == 0xff42 && cycle == 2) {  //SCY
    return status.scy;
  }

  if(address == 0xff43 && cycle == 2) {  //SCX
    return status.scx;
  }

  if(address == 0xff44 && cycle == 2) {  //LY
    return status.ly;
  }

  if(address == 0xff45 && cycle == 2) {  //LYC
    return status.lyc;
  }

  if(address == 0xff46 && cycle == 2) {  //DMA
    data = status.dmaBank;
  }

  if(address == 0xff47 && cycle == 2) {  //BGP
    data.bit(0,1) = bgp[0];
    data.bit(2,3) = bgp[1];
    data.bit(4,5) = bgp[2];
    data.bit(6,7) = bgp[3];
    return data;
  }

  if(address == 0xff48 && cycle == 2) {  //OBP0
    data.bit(0,1) = obp[0][0];
    data.bit(2,3) = obp[1][1];
    data.bit(4,5) = obp[2][2];
    data.bit(6,7) = obp[3][3];
    return data;
  }

  if(address == 0xff49 && cycle == 2) {  //OBP1
    data.bit(0,1) = obp[1][0];
    data.bit(2,3) = obp[1][1];
    data.bit(4,5) = obp[1][2];
    data.bit(6,7) = obp[1][3];
    return data;
  }

  if(address == 0xff4a && cycle == 2) {  //WY
    return status.wy;
  }

  if(address == 0xff4b && cycle == 2) {  //WX
    return status.wx;
  }

  if(Model::GameBoyColor())
  if(address == 0xff4f && cycle == 2) {  //VBK
    return status.vramBank;
  }

  if(Model::GameBoyColor())
  if(address == 0xff68 && cycle == 2) {  //BGPI
    data.bit(0,5) = status.bgpi;
    data.bit(7)   = status.bgpiIncrement;
    return data;
  }

  if(Model::GameBoyColor())
  if(address == 0xff69 && cycle == 2) {  //BGPD
    return bgpd[status.bgpi];
  }

  if(Model::GameBoyColor())
  if(address == 0xff6a && cycle == 2) {  //OBPI
    data.bit(0,5) = status.obpi;
    data.bit(7)   = status.obpiIncrement;
    return data;
  }

  if(Model::GameBoyColor())
  if(address == 0xff6b && cycle == 2) {  //OBPD
    return obpd[status.obpi];
  }

  return data;
}

auto PPU::writeIO(uint cycle, uint16 address, uint8 data) -> void {
  if(address >= 0x8000 && address <= 0x9fff && cycle == 2) {
    vram[vramAddress(address)] = data;
    return;
  }

  if(address >= 0xfe00 && address <= 0xfe9f && cycle == 2) {
    if(status.dmaActive && status.dmaClock >= 8) return;
    oam[(uint8)address] = data;
    return;
  }

  if(address < 0xff40 || address > 0xff7f) return;

  if(address == 0xff40 && cycle == 4) {  //LCDC
    if(status.displayEnable != data.bit(7)) {
      status.mode = 0;
      status.ly = 0;
      status.lx = 0;
      if(data.bit(7)) latch.displayEnable = 1;

      //restart cothread to begin new frame
      auto clock = Thread::clock();
      Thread::create(4 * 1024 * 1024, {&PPU::main, this});
      Thread::setClock(clock);
    }

    status.bgEnable            = data.bit(0);
    status.obEnable            = data.bit(1);
    status.obSize              = data.bit(2);
    status.bgTilemapSelect     = data.bit(3);
    status.bgTiledataSelect    = data.bit(4);
    status.windowDisplayEnable = data.bit(5);
    status.windowTilemapSelect = data.bit(6);
    status.displayEnable       = data.bit(7);
    return;
  }

  if(!Model::GameBoyColor())
  if(address == 0xff41 && cycle == 2) {  //STAT
    //hardware bug: writes to STAT on non-CGB hardware force-enable flags for 1 T-cycle.
    //this can generate spurious interrupts if the STAT line is currently low.
    status.interruptHblank = 1;
    status.interruptVblank = 1;
    status.interruptOAM    = 1;
    status.interruptLYC    = 1;
    return;
  }

  if(address == 0xff41 && cycle == 4) {  //STAT
    status.interruptHblank = data.bit(3);
    status.interruptVblank = data.bit(4);
    status.interruptOAM    = data.bit(5);
    status.interruptLYC    = data.bit(6);
    return;
  }

  if(address == 0xff42 && cycle == 2) {  //SCY
    status.scy = data;
    return;
  }

  if(address == 0xff43 && cycle == 2) {  //SCX
    status.scx = data;
    return;
  }

  if(address == 0xff44 && cycle == 2) {  //LY
    status.ly = 0;
    return;
  }

  if(address == 0xff45 && cycle == 2) {  //LYC
    status.lyc = data;
    return;
  }

  if(address == 0xff46 && cycle == 2) {  //DMA
    status.dmaBank   = data;
    status.dmaActive = 1;
    status.dmaClock  = 0;
    return;
  }

  if(address == 0xff47 && cycle == 2) {  //BGP
    bgp[0] = data.bit(0,1);
    bgp[1] = data.bit(2,3);
    bgp[2] = data.bit(4,5);
    bgp[3] = data.bit(6,7);
    return;
  }

  if(address == 0xff48 && cycle == 2) {  //OBP0
    obp[0][0] = data.bit(0,1);
    obp[0][1] = data.bit(2,3);
    obp[0][2] = data.bit(4,5);
    obp[0][3] = data.bit(6,7);
    return;
  }

  if(address == 0xff49 && cycle == 2) {  //OBP1
    obp[1][0] = data.bit(0,1);
    obp[1][1] = data.bit(2,3);
    obp[1][2] = data.bit(4,5);
    obp[1][3] = data.bit(6,7);
    return;
  }

  if(address == 0xff4a && cycle == 2) {  //WY
    status.wy = data;
    return;
  }

  if(address == 0xff4b && cycle == 2) {  //WX
    status.wx = data;
    return;
  }

  if(Model::GameBoyColor())
  if(address == 0xff4f && cycle == 2) {  //VBK
    status.vramBank = data.bit(0);
    return;
  }

  if(Model::GameBoyColor())
  if(address == 0xff68 && cycle == 2) {  //BGPI
    status.bgpi          = data.bit(0,5);
    status.bgpiIncrement = data.bit(7);
    return;
  }

  if(Model::GameBoyColor())
  if(address == 0xff69 && cycle == 2) {  //BGPD
    bgpd[status.bgpi] = data;
    if(status.bgpiIncrement) status.bgpi++;
    return;
  }

  if(Model::GameBoyColor())
  if(address == 0xff6a && cycle == 2) {  //OBPI
    status.obpi          = data.bit(0,5);
    status.obpiIncrement = data.bit(7);
  }

  if(Model::GameBoyColor())
  if(address == 0xff6b && cycle == 2) {  //OBPD
    obpd[status.obpi] = data;
    if(status.obpiIncrement) status.obpi++;
  }
}
