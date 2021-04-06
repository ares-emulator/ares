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
  vblank = 1;
  selectFramebuffer(framebufferSelect);
}

auto M32X::VDP::scanline(u32 pixels[1280], u32 y) -> void {
  if(!Mega32X() || !pixels) return;
  if(mode == 1) return scanlineMode1(pixels, y);
  if(mode == 2) return scanlineMode2(pixels, y);
  if(mode == 3) return scanlineMode3(pixels, y);
}

auto M32X::VDP::scanlineMode1(u32 pixels[1280], u32 y) -> void {
  u16 address = fbram[y];
  for(u32 x = dotshift; x < 320 + dotshift; x++) {
    u8 color = fbram[address + (x >> 1) & 0xffff].byte(!(x & 1));
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
    u8 length = (word >> 8) + 1;
    u8 color  = (word >> 0);
    u16 pixel = cram[color];
    for(u32 repeat : range(length)) {
      plot(&pixels[x * 4], pixel);
      x++;
    }
  }
}

auto M32X::VDP::plot(u32* output, u16 color) -> void {
  n1 backdrop = output[0] >> 11;
  n1 throughbit = color >> 15;
  b1 opaque = color & 0x7fff;

  if(priority == 0) {
    //Mega Drive has priority
    if(throughbit || backdrop) {
      output[0] = color | 1 << 15;
      output[1] = color | 1 << 15;
      output[2] = color | 1 << 15;
      output[3] = color | 1 << 15;
    }
  } else {
    //Mega 32X has priority
    if(!throughbit || backdrop) {
      output[0] = color | 1 << 15;
      output[1] = color | 1 << 15;
      output[2] = color | 1 << 15;
      output[3] = color | 1 << 15;
    }
  }
}

auto M32X::VDP::fill() -> void {
  for(u32 repeat : range(1 + autofillLength)) {
    bbram[autofillAddress] = autofillData;
    autofillAddress.byte(0)++;
  }
}

auto M32X::VDP::selectFramebuffer(n1 select) -> void {
  framebufferSelect = select;
  if(!vblank && mode) return;

  framebufferActive = select;
  fbram = {dram.data() + 0x10000 * (select == 0), 0x10000};
  bbram = {dram.data() + 0x10000 * (select == 1), 0x10000};
}
