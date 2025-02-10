auto M32X::VDP::load(Node::Object parent) -> void {
  dram.allocate(512_KiB >> 1);
  cram.allocate(512 >> 1);
  debugger.load(parent);
}

auto M32X::VDP::unload() -> void {
  debugger = {};
  dram.reset();
  cram.reset();
}

auto M32X::VDP::power(bool reset) -> void {
  dram.fill(0);
  cram.fill(0);
  mode = 0;
  lines = 0;
  priority = 0;
  dotshift = 0;
  latch = {};
  autofillLength = 0;
  autofillAddress = 0;
  autofillData = 0;
  framebufferAccess = 0;
  framebufferActive = 0;
  framebufferSelect = 0;
  framebufferWait = 0;
  hblank = 0;
  vblank = 1;
  selectFramebuffer(framebufferSelect);
}

auto M32X::VDP::scanline(u32 pixels[1280], u32 y) -> void {
  if(!Mega32X() || !pixels || y >= (latch.lines ? 240 : 224)) return;
  if(latch.mode == 1) return scanlineMode1(pixels, y);
  if(latch.mode == 2) return scanlineMode2(pixels, y);
  if(latch.mode == 3) return scanlineMode3(pixels, y);
}

auto M32X::VDP::scanlineMode1(u32 pixels[1280], u32 y) -> void {
  u16 address = fbram[y];
  for(u32 x : range(320)) {
    u8 color = fbram[address + (x + latch.dotshift >> 1) & 0xffff].byte(!(x + latch.dotshift & 1));
    plot(&pixels[x * 4], cram[color]);
  }
}

auto M32X::VDP::scanlineMode2(u32 pixels[1280], u32 y) -> void {
  u16 address = fbram[y];
  for(u32 x : range(320)) {
    u16 pixel = fbram[address++ & 0xffff];
    plot(&pixels[x * 4], pixel);
  }
}

auto M32X::VDP::scanlineMode3(u32 pixels[1280], u32 y) -> void {
  u16 address = fbram[y];
  for(u32 x = 0; x < 320;) {
    u16 word  = fbram[address++ & 0xffff];
    u8 length = word >> 8;
    u8 color  = word >> 0;
    u16 pixel = cram[color];
    for(u32 repeat : range(min(length+1, 320-x))) {
      plot(&pixels[x * 4], pixel);
      x++;
    }
  }
}

auto M32X::VDP::plot(u32* output, u16 color) -> void {
  n1 throughbit = color >> 15;

  for(int i = 0; i < 4; i++) {
    n1 backdrop = output[i] >> 11;

    if(latch.priority == 0) {
      //Mega Drive has priority
      if(throughbit || backdrop) output[i] = color | 1 << 15;
    } else {
      //Mega 32X has priority
      if(!throughbit || backdrop) output[i] = color | 1 << 15;
    }
  }
}

auto M32X::VDP::fill() -> void {
  if(framebufferWait > 0) { debug(unusual, "[32X FILL] triggered before last fill finished"); return; }
  framebufferWait = 7+3*(autofillLength+1); // according to official docs
  for(u32 repeat : range(1 + autofillLength)) {
    bbram[autofillAddress] = autofillData;
    autofillAddress.byte(0)++;
  }
}

auto M32X::VDP::selectFramebuffer(n1 select) -> void {
  framebufferSelect = select;
  if(!vblank && latch.mode) return;

  framebufferActive = select;
  fbram = {dram.data() + 0x10000 * (select == 0), 0x10000};
  bbram = {dram.data() + 0x10000 * (select == 1), 0x10000};
}

// back buffer access
auto M32X::VDP::framebufferEngaged() -> bool {
  // TODO: 40 cycle wait at start of hblank (according to official docs)
  return (latch.mode != 0 && framebufferSelect != framebufferActive) || framebufferWait > 0;
}

auto M32X::VDP::paletteEngaged() -> bool {
  // TODO: not available for 24 MClks (~10 cycles) at start of hblank (according to official docs)
  return !vblank && !hblank && latch.mode.bit(0);
}
