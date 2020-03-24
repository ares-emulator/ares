auto PPU::portRead(uint16 address) -> uint8 {
  uint8 data;

  //DISP_CTRL
  if(address == 0x0000) {
    data.bit(0) = r.screenOneEnable;
    data.bit(1) = r.screenTwoEnable;
    data.bit(2) = r.spriteEnable;
    data.bit(3) = r.spriteWindowEnable;
    data.bit(4) = r.screenTwoWindowInvert;
    data.bit(5) = r.screenTwoWindowEnable;
    return data;
  }

  //BACK_COLOR
  if(address == 0x0001) {
    if(grayscale()) {
      data.bit(0,2) = r.backColor.bit(0,2);
    } else {
      data.bit(0,7) = r.backColor.bit(0,7);
    }
    return data;
  }

  //LINE_CUR
  if(address == 0x0002) {
    //todo: unknown if this is vtime or vtime%(vtotal+1)
    return s.vtime;
  }

  //LINE_CMP
  if(address == 0x0003) {
    return r.lineCompare;
  }

  //SPR_BASE
  if(address == 0x0004) {
    if(depth() == 2) {
      data.bit(0,4) = r.spriteBase.bit(0,4);
    } else {
      data.bit(0,5) = r.spriteBase.bit(0,5);
    }
    return data;
  }

  //SPR_FIRST
  if(address == 0x0005) {
    return r.spriteFirst;
  }

  //SPR_COUNT
  if(address == 0x0006) {
    return r.spriteCount;
  }

  //MAP_BASE
  if(address == 0x0007) {
    if(depth() == 2) {
      data.bit(0,2) = r.screenOneMapBase.bit(0,2);
      data.bit(4,6) = r.screenTwoMapBase.bit(0,2);
    } else {
      data.bit(0,3) = r.screenOneMapBase.bit(0,3);
      data.bit(4,7) = r.screenTwoMapBase.bit(0,3);
    }
    return data;
  }

  //SCR2_WIN_X0
  if(address == 0x0008) {
    return r.screenTwoWindowX0;
  }

  //SCR2_WIN_Y0
  if(address == 0x0009) {
    return r.screenTwoWindowY0;
  }

  //SCR2_WIN_X1
  if(address == 0x000a) {
    return r.screenTwoWindowX1;
  }

  //SCR2_WIN_Y1
  if(address == 0x000b) {
    return r.screenTwoWindowY1;
  }

  //SPR_WIN_X0
  if(address == 0x000c) {
    return r.spriteWindowX0;
  }

  //SPR_WIN_Y0
  if(address == 0x000d) {
    return r.spriteWindowY0;
  }

  //SPR_WIN_X1
  if(address == 0x000e) {
    return r.spriteWindowX1;
  }

  //SPR_WIN_Y1
  if(address == 0x000f) {
    return r.spriteWindowY1;
  }

  //SCR1_X
  if(address == 0x0010) {
    return r.scrollOneX;
  }

  //SCR1_Y
  if(address == 0x0011) {
    return r.scrollOneY;
  }

  //SCR2_X
  if(address == 0x0012) {
    return r.scrollTwoX;
  }

  //SCR2_Y
  if(address == 0x0013) {
    return r.scrollTwoY;
  }

  //LCD_CTRL
  if(address == 0x0014) {
    data.bit(0) = r.lcdEnable;
    if(SoC::ASWAN()) {
      data.bit(1,7) = r.lcdUnknown.bit(1,7);
    }
    if(SoC::SPHINX()) {
      data.bit(1)   = r.lcdContrast;
      data.bit(4,7) = r.lcdUnknown.bit(4,7);
    }
    return data;
  }

  //LCD_ICON
  if(address == 0x0015) {
    data.bit(0) = r.icon.sleeping;
    data.bit(1) = r.icon.orientation1;
    data.bit(2) = r.icon.orientation0;
    data.bit(3) = r.icon.auxiliary0;
    data.bit(4) = r.icon.auxiliary1;
    data.bit(5) = r.icon.auxiliary2;
    return data;
  }

  //LCD_VTOTAL
  if(address == 0x0016) {
    return r.vtotal;
  }

  //LCD_VSYNC
  if(address == 0x0017) {
    return r.vsync;
  }

  //PALMONO_POOL
  if(address >= 0x001c && address <= 0x001f) {
    data.bit(0,3) = r.pool[address.bit(0,1) << 1 | 0];
    data.bit(4,7) = r.pool[address.bit(0,1) << 1 | 1];
    return data;
  }

  //PALMONO
  if(address >= 0x0020 && address <= 0x003f) {
    data.bit(0,3) = r.palette[address.bit(1,4)].color[address.bit(0) << 1 | 0];
    data.bit(4,7) = r.palette[address.bit(1,4)].color[address.bit(0) << 1 | 1];
    return data;
  }

  //TMR_CTRL
  if(address == 0x00a2) {
    data.bit(0) = r.htimerEnable;
    data.bit(1) = r.htimerRepeat;
    data.bit(2) = r.vtimerEnable;
    data.bit(3) = r.vtimerRepeat;
    return data;
  }

  //HTMR_FREQ
  if(address == 0x00a4 || address == 0x00a5) {
    return r.htimerFrequency.byte(address & 1);
  }

  //VTMR_FREQ
  if(address == 0x00a6 || address == 0x00a7) {
    return r.vtimerFrequency.byte(address & 1);
  }

  //HTMR_CTR
  if(address == 0x00a8 || address == 0x00a9) {
    return r.htimerCounter.byte(address & 1);
  }

  //VTMR_CTR
  if(address == 0x00aa || address == 0x00ab) {
    return r.vtimerCounter.byte(address & 1);
  }

  return data;
}

auto PPU::portWrite(uint16 address, uint8 data) -> void {
  //DISP_CTRL
  if(address == 0x0000) {
    r.screenOneEnable       = data.bit(0);
    r.screenTwoEnable       = data.bit(1);
    r.spriteEnable          = data.bit(2);
    r.spriteWindowEnable    = data.bit(3);
    r.screenTwoWindowInvert = data.bit(4);
    r.screenTwoWindowEnable = data.bit(5);
    return;
  }

  //BACK_COLOR
  if(address == 0x0001) {
    r.backColor = data;
    return;
  }

  //LINE_CMP
  if(address == 0x0003) {
    r.lineCompare = data;
    return;
  }

  //SPR_BASE
  if(address == 0x0004) {
    r.spriteBase = data.bit(0,5);
    return;
  }

  //SPR_FIRST
  if(address == 0x0005) {
    r.spriteFirst = data.bit(6,0);
    return;
  }

  //SPR_COUNT
  if(address == 0x0006) {
    r.spriteCount = data;
    return;
  }

  //MAP_BASE
  if(address == 0x0007) {
    r.screenOneMapBase = data.bit(0,3);
    r.screenTwoMapBase = data.bit(4,7);
    return;
  }

  //SCR2_WIN_X0
  if(address == 0x0008) {
    r.screenTwoWindowX0 = data;
    return;
  }

  //SCR2_WIN_Y0
  if(address == 0x0009) {
    r.screenTwoWindowY0 = data;
    return;
  }

  //SCR2_WIN_X1
  if(address == 0x000a) {
    r.screenTwoWindowX1 = data;
    return;
  }

  //SCR2_WIN_Y1
  if(address == 0x000b) {
    r.screenTwoWindowY1 = data;
    return;
  }

  //SPR_WIN_X0
  if(address == 0x000c) {
    r.spriteWindowX0 = data;
    return;
  }

  //SPR_WIN_Y0
  if(address == 0x000d) {
    r.spriteWindowY0 = data;
    return;
  }

  //SPR_WIN_X1
  if(address == 0x000e) {
    r.spriteWindowX1 = data;
    return;
  }

  //SPR_WIN_Y1
  if(address == 0x000f) {
    r.spriteWindowY1 = data;
    return;
  }

  //SCR1_X
  if(address == 0x0010) {
    r.scrollOneX = data;
    return;
  }

  //SCR1_Y
  if(address == 0x0011) {
    r.scrollOneY = data;
    return;
  }

  //SCR2_X
  if(address == 0x0012) {
    r.scrollTwoX = data;
    return;
  }

  //SCR2_Y
  if(address == 0x0013) {
    r.scrollTwoY = data;
    return;
  }

  //LCD_CTRL
  if(address == 0x0014) {
    r.lcdEnable = data.bit(0);
    if(SoC::ASWAN()) {
      r.lcdUnknown.bit(1,7) = data.bit(1,7);
    }
    if(SoC::SPHINX()) {
      r.lcdContrast = data.bit(1);
      r.lcdUnknown.bit(4,7) = data.bit(4,7);
    }
    return;
  }

  //LCD_ICON
  if(address == 0x0015) {
    r.icon.sleeping     = data.bit(0);
    r.icon.orientation1 = data.bit(1);
    r.icon.orientation0 = data.bit(2);
    r.icon.auxiliary0   = data.bit(3);
    r.icon.auxiliary1   = data.bit(4);
    r.icon.auxiliary2   = data.bit(5);
    updateIcons();
    updateOrientation();
    return;
  }

  //LCD_VTOTAL
  if(address == 0x0016) {
    r.vtotal = data;
    return;
  }

  //LCD_VSYNC
  if(address == 0x0017) {
    r.vsync = data;
    return;
  }

  //PALMONO_POOL
  if(address >= 0x001c && address <= 0x001f) {
    r.pool[address.bit(0,1) << 1 | 0] = data.bit(0,3);
    r.pool[address.bit(0,1) << 1 | 1] = data.bit(4,7);
    return;
  }

  //PALMONO
  if(address >= 0x0020 && address <= 0x003f) {
    r.palette[address.bit(1,4)].color[address.bit(0) << 1 | 0] = data.bit(0,2);
    r.palette[address.bit(1,4)].color[address.bit(0) << 1 | 1] = data.bit(4,6);
    return;
  }

  //TMR_CTRL
  if(address == 0x00a2) {
    r.htimerEnable = data.bit(0);
    r.htimerRepeat = data.bit(1);
    r.vtimerEnable = data.bit(2);
    r.vtimerRepeat = data.bit(3);
    if(r.htimerEnable) r.htimerCounter = 0;
    if(r.vtimerEnable) r.vtimerCounter = 0;
    return;
  }

  //HTMR_FREQ
  if(address == 0x00a4 || address == 0x00a5) {
    r.htimerFrequency.byte(address & 1) = data;
    r.htimerEnable = 1;
    r.htimerRepeat = 1;
    r.htimerCounter = 0;
  }

  //VTMR_FREQ
  if(address == 0x00a6 || address == 0x00a7) {
    r.vtimerFrequency.byte(address & 1) = data;
    r.vtimerEnable = 1;
    r.vtimerRepeat = 1;
    r.vtimerCounter = 0;
  }
}
