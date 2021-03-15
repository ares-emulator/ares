auto M32X::scanline(u32 pixels[1280], u32 y) -> void {
  if(!pixels) return;
  if(vdp.mode == 1) return scanlineMode1(pixels, y);
  if(vdp.mode == 2) return scanlineMode2(pixels, y);
  if(vdp.mode == 3) return scanlineMode3(pixels, y);
}

auto M32X::scanlineMode1(u32 pixels[1280], u32 y) -> void {
  u16 address = dram[y];
  for(u32 x : range(320)) {
    u8 color = dram[address + (x >> 1)].byte(!(x & 1));
    u16 pixel = cram[color];
    u8 r = pixel >>  2 & 7;
    u8 g = pixel >>  7 & 7;
    u8 b = pixel >> 12 & 7;
    n1 a = pixel >> 15 & 1;
    pixel = r << 0 | g << 3 | b << 6 | 1 << 9;

    if(vdp.priority != a || n9(pixels[x * 4]) == 0) {
      pixels[x * 4 + 0] = pixel;
      pixels[x * 4 + 1] = pixel;
      pixels[x * 4 + 2] = pixel;
      pixels[x * 4 + 3] = pixel;
    }
  }
}

auto M32X::scanlineMode2(u32 pixels[1280], u32 y) -> void {
  u16 address = dram[y];
  for(u32 x : range(320)) {
    u16 pixel = dram[address++];
    u8 r = pixel >>  2 & 7;
    u8 g = pixel >>  7 & 7;
    u8 b = pixel >> 12 & 7;
    n1 a = pixel >> 15 & 1;
    pixel = r << 0 | g << 3 | b << 6 | 1 << 9;

    if(vdp.priority != a || n9(pixels[x * 4]) == 0) {
      pixels[x * 4 + 0] = pixel;
      pixels[x * 4 + 1] = pixel;
      pixels[x * 4 + 2] = pixel;
      pixels[x * 4 + 3] = pixel;
    }
  }
}

auto M32X::scanlineMode3(u32 pixels[1280], u32 y) -> void {
  u16 address = dram[y];
  for(u32 x = 0; x < 320;) {
    u16 word = dram[address++];
    u8 length = (word >> 8) + 1;
    u8 color  = (word >> 0);
    u16 pixel = cram[color];
    u8 r = pixel >>  2 & 7;
    u8 g = pixel >>  7 & 7;
    u8 b = pixel >> 12 & 7;
    n1 a = pixel >> 15 & 1;
    pixel = r << 0 | g << 3 | b << 6 | 1 << 9;
    for(u32 r : range(word >> 8)) {
      if(vdp.priority != a || n9(pixels[x * 4]) == 0) {
        pixels[x * 4 + 0] = pixel;
        pixels[x * 4 + 1] = pixel;
        pixels[x * 4 + 2] = pixel;
        pixels[x * 4 + 3] = pixel;
      }
      x++;
    }
  }
}

auto M32X::vdpFill() -> void {
  for(u32 address : range(1 + vdp.autofillLength)) {
    dram[vdp.autofillAddress + address] = vdp.autofillData;
  }
}

auto M32X::vdpDMA() -> void {
  dreq.active = 0;
  print("* 32X DMA\n");
}
