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
  selectFramebuffer(0);
}

auto M32X::VDP::scanline(u32 pixels[1280], u32 y) -> void {
  if(!pixels) return;
  if(mode == 1) return scanlineMode1(pixels, y);
  if(mode == 2) return scanlineMode2(pixels, y);
  if(mode == 3) return scanlineMode3(pixels, y);
}

auto M32X::VDP::scanlineMode1(u32 pixels[1280], u32 y) -> void {
  u16 address = fbram[y];
  for(u32 x : range(320)) {
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

auto M32X::VDP::plot(u32* target, u16 color) -> void {
  n9 source = target[0];   //get Mega Drive M2B3G3R3 pixel and clip M2 bits
  n1 alpha = color >> 15;  //extract A1R5G5B5 alpha bit
  color += 3 * (1 << 9);   //add Mega 32X palette index offset

  if(priority == 0) {
    //Mega Drive has priority
    if(source == 0 || alpha == 1) {
      target[0] = color;
      target[1] = color;
      target[2] = color;
      target[3] = color;
    }
  } else {
    //Megas 32X has priority
    if(alpha == 0 || source == 0) {
      target[0] = color;
      target[1] = color;
      target[2] = color;
      target[3] = color;
    }
  }
}

auto M32X::VDP::fill() -> void {
  for(u32 repeat : range(1 + autofillLength)) {
    bbram[autofillAddress] = autofillData;
    autofillAddress.byte(0)++;
  }
}

auto M32X::VDP::selectFramebuffer(n1 active) -> void {
  framebufferSelect = active;
  fbram = {dram.data() + 0x10000 * (framebufferSelect == 0), 0x10000};
  bbram = {dram.data() + 0x10000 * (framebufferSelect == 1), 0x10000};
}
