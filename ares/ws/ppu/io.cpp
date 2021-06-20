auto PPU::readIO(n16 address) -> n8 {
  n8 data;

  switch(address) {

  case 0x0000:  //DISP_CTRL
    data.bit(0) = screen1.enable[1];
    data.bit(1) = screen2.enable[1];
    data.bit(2) = sprite.enable[1];
    data.bit(3) = sprite.window.enable[1];
    data.bit(4) = screen2.window.invert[1];
    data.bit(5) = screen2.window.enable[1];
    break;

  case 0x0001:  //BACK_COLOR
    if(grayscale()) {
      data.bit(0,2) = dac.backdrop[1].bit(0,2);
    } else {
      data.bit(0,7) = dac.backdrop[1].bit(0,7);
    }
    break;

  case 0x0002:  //LINE_CUR
    //todo: unknown if this is vcounter or vcounter%(vtotal+1)
    data = io.vcounter;
    break;

  case 0x0003:  //LINE_CMP
    data = io.vcompare;
    break;

  case 0x0004:  //SPR_BASE
    if(depth() == 2) {
      data.bit(0,4) = sprite.oamBase.bit(0,4);
    } else {
      data.bit(0,5) = sprite.oamBase.bit(0,5);
    }
    break;

  case 0x0005:  //SPR_FIRST
    data = sprite.first;
    break;

  case 0x0006:  //SPR_COUNT
    data = sprite.count;
    break;

  case 0x0007:  //MAP_BASE
    if(depth() == 2) {
      data.bit(0,2) = screen1.mapBase[1].bit(0,2);
      data.bit(4,6) = screen2.mapBase[1].bit(0,2);
    } else {
      data.bit(0,3) = screen1.mapBase[1].bit(0,3);
      data.bit(4,7) = screen2.mapBase[1].bit(0,3);
    }
    break;

  case 0x0008:  //SCR2_WIN_X0
    data = screen2.window.x0[1];
    break;

  case 0x0009:  //SCR2_WIN_Y0
    data = screen2.window.y0[1];
    break;

  case 0x000a:  //SCR2_WIN_X1
    data = screen2.window.x1[1];
    break;

  case 0x000b:  //SCR2_WIN_Y1
    data = screen2.window.y1[1];
    break;

  case 0x000c:  //SPR_WIN_X0
    data = sprite.window.x0[1];
    break;

  case 0x000d:  //SPR_WIN_Y0
    data = sprite.window.y0[1];
    break;

  case 0x000e:  //SPR_WIN_X1
    data = sprite.window.x1[1];
    break;

  case 0x000f:  //SPR_WIN_Y1
    data = sprite.window.y1[1];
    break;

  case 0x0010:  //SCR1_X
    data = screen1.hscroll[1];
    break;

  case 0x0011:  //SCR1_Y
    data = screen1.vscroll[1];
    break;

  case 0x0012:  //SCR2_X
    data = screen2.hscroll[1];
    break;

  case 0x0013:  //SCR2_Y
    data = screen2.vscroll[1];
    break;

  case 0x0014:  //LCD_CTRL
    data.bit(0) = dac.enable[1];
    if(SoC::ASWAN()) {
      data.bit(1,7) = dac.unknown.bit(1,7);
    }
    if(SoC::SPHINX()) {
      data.bit(1)   = dac.contrast[1];
      data.bit(4,7) = dac.unknown.bit(4,7);
    }
    break;

  case 0x0015:  //LCD_ICON
    data.bit(0) = lcd.icon.sleeping;
    data.bit(1) = lcd.icon.orientation1;
    data.bit(2) = lcd.icon.orientation0;
    data.bit(3) = lcd.icon.auxiliary0;
    data.bit(4) = lcd.icon.auxiliary1;
    data.bit(5) = lcd.icon.auxiliary2;
    break;

  case 0x0016:  //LCD_VTOTAL
    data = io.vtotal;
    break;

  case 0x0017:  //LCD_VSYNC
    data = io.vsync;
    break;

  case 0x001c ... 0x001f:  //PALMONO_POOL
    data.bit(0,3) = pram.pool[address.bit(0,1) << 1 | 0];
    data.bit(4,7) = pram.pool[address.bit(0,1) << 1 | 1];
    break;

  case 0x0020 ... 0x003f:  //PALMONO
    data.bit(0,3) = pram.palette[address.bit(1,4)].color[address.bit(0) << 1 | 0];
    data.bit(4,7) = pram.palette[address.bit(1,4)].color[address.bit(0) << 1 | 1];
    break;

  case 0x00a2:  //TMR_CTRL
    data.bit(0) = htimer.enable;
    data.bit(1) = htimer.repeat;
    data.bit(2) = vtimer.enable;
    data.bit(3) = vtimer.repeat;
    break;

  case 0x00a4 ... 0x00a5:  //HTMR_FREQ
    data = htimer.frequency.byte(address - 0x00a4);
    break;

  case 0x00a6 ... 0x00a7:  //VTMR_FREQ
    data = vtimer.frequency.byte(address - 0x00a6);
    break;

  case 0x00a8 ... 0x00a9:  //HTMR_CTR
    data = htimer.counter.byte(address - 0x00a8);
    break;

  case 0x00aa ... 0x00ab:  //VTMR_CTR
    data = vtimer.counter.byte(address - 0x00aa);
    break;

  }

  return data;
}

auto PPU::writeIO(n16 address, n8 data) -> void {
  switch(address) {

  case 0x0000:  //DISP_CTRL
    screen1.enable[1]        = data.bit(0);
    screen2.enable[1]        = data.bit(1);
    sprite.enable[1]         = data.bit(2);
    sprite.window.enable[1]  = data.bit(3);
    screen2.window.invert[1] = data.bit(4);
    screen2.window.enable[1] = data.bit(5);
    break;

  case 0x0001:  //BACK_COLOR
    dac.backdrop[1] = data;
    break;

  case 0x0003:  //LINE_CMP
    io.vcompare = data;
    break;

  case 0x0004:  //SPR_BASE
    sprite.oamBase = data.bit(0,5);
    break;

  case 0x0005:  //SPR_FIRST
    sprite.first = data.bit(0,6);
    break;

  case 0x0006:  //SPR_COUNT
    sprite.count = data;
    break;

  case 0x0007:  //MAP_BASE
    screen1.mapBase[1] = data.bit(0,3);
    screen2.mapBase[1] = data.bit(4,7);
    break;

  case 0x0008:  //SCR2_WIN_X0
    screen2.window.x0[1] = data;
    break;

  case 0x0009:  //SCR2_WIN_Y0
    screen2.window.y0[1] = data;
    break;

  case 0x000a:  //SCR2_WIN_X1
    screen2.window.x1[1] = data;
    break;

  case 0x000b:  //SCR2_WIN_Y1
    screen2.window.y1[1] = data;
    break;

  case 0x000c:  //SPR_WIN_X0
    sprite.window.x0[1] = data;
    break;

  case 0x000d:  //SPR_WIN_Y0
    sprite.window.y0[1] = data;
    break;

  case 0x000e:  //SPR_WIN_X1
    sprite.window.x1[1] = data;
    break;

  case 0x000f:  //SPR_WIN_Y1
    sprite.window.y1[1] = data;
    break;

  case 0x0010:  //SCR1_X
    screen1.hscroll[1] = data;
    break;

  case 0x0011:  //SCR1_Y
    screen1.vscroll[1] = data;
    break;

  case 0x0012:  //SCR2_X
    screen2.hscroll[1] = data;
    break;

  case 0x0013:  //SCR2_Y
    screen2.vscroll[1] = data;
    break;

  case 0x0014:  //LCD_CTRL
    dac.enable[1] = data.bit(0);
    if(SoC::ASWAN()) {
      dac.unknown.bit(1,7) = data.bit(1,7);
    }
    if(SoC::SPHINX()) {
      dac.contrast[1]      = data.bit(1);
      dac.unknown.bit(4,7) = data.bit(4,7);
    }
    break;

  case 0x0015:  //LCD_ICON
    lcd.icon.sleeping     = data.bit(0);
    lcd.icon.orientation1 = data.bit(1);
    lcd.icon.orientation0 = data.bit(2);
    lcd.icon.auxiliary0   = data.bit(3);
    lcd.icon.auxiliary1   = data.bit(4);
    lcd.icon.auxiliary2   = data.bit(5);
    updateIcons();
    updateOrientation();
    break;

  case 0x0016:  //LCD_VTOTAL
    io.vtotal = data;
    break;

  case 0x0017:  //LCD_VSYNC
    io.vsync = data;
    break;

  case 0x001c ... 0x001f:  //PALMONO_POOL
    pram.pool[address.bit(0,1) << 1 | 0] = data.bit(0,3);
    pram.pool[address.bit(0,1) << 1 | 1] = data.bit(4,7);
    break;

  case 0x0020 ... 0x003f:  //PALMONO
    pram.palette[address.bit(1,4)].color[address.bit(0) << 1 | 0] = data.bit(0,2);
    pram.palette[address.bit(1,4)].color[address.bit(0) << 1 | 1] = data.bit(4,6);
    break;

  case 0x00a2:  //TMR_CTRL
    if(!htimer.enable && data.bit(0)) htimer.counter = htimer.frequency;
    if(!vtimer.enable && data.bit(1)) vtimer.counter = vtimer.frequency;
    htimer.enable = data.bit(0);
    htimer.repeat = data.bit(1);
    vtimer.enable = data.bit(2);
    vtimer.repeat = data.bit(3);
    break;

  case 0x00a4 ... 0x00a5:  //HTMR_FREQ
    htimer.frequency.byte(address - 0x00a4) = data;
    htimer.counter  .byte(address - 0x00a4) = data;
    break;

  case 0x00a6 ... 0x00a7:  //VTMR_FREQ
    vtimer.frequency.byte(address - 0x00a6) = data;
    vtimer.counter  .byte(address - 0x00a6) = data;
    break;

  }

  return;
}
