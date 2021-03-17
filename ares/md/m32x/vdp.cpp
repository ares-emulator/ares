auto M32X::scanline(u32 pixels[1280], u32 y) -> void {
  if(!pixels) return;
  if(vdp.mode == 1) return scanlineMode1(pixels, y);
  if(vdp.mode == 2) return scanlineMode2(pixels, y);
  if(vdp.mode == 3) return scanlineMode3(pixels, y);
}

auto M32X::plot(u32* target, u16 color) -> void {
  n9 source = target[0];   //get Mega Drive M2B3G3R3 pixel and clip M2 bits
  n1 alpha = color >> 15;  //extract A1R5G5B5 alpha bit
  color += 3 * (1 << 9);   //add Mega 32X palette index offset

  if(vdp.priority == 0) {
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

auto M32X::scanlineMode1(u32 pixels[1280], u32 y) -> void {
  u16 address = dram[y];
  for(u32 x : range(320)) {
    u8 color = dram[address + (x >> 1)].byte(!(x & 1));
    plot(&pixels[x * 4], cram[color]);
  }
}

auto M32X::scanlineMode2(u32 pixels[1280], u32 y) -> void {
  u16 address = dram[y];
  for(u32 x : range(320)) {
    plot(&pixels[x * 4], dram[address++]);
  }
}

auto M32X::scanlineMode3(u32 pixels[1280], u32 y) -> void {
  u16 address = dram[y];
  for(u32 x = 0; x < 320;) {
    u16 word = dram[address++];
    u8 length = (word >> 8) + 1;
    u8 color  = (word >> 0);
    u16 pixel = cram[color];
    for(u32 repeat : range(length)) {
      plot(&pixels[x * 4], pixel);
      x++;
    }
  }
}

auto M32X::vdpFill() -> void {
  for(u32 repeat : range(1 + vdp.autofillLength)) {
    dram[vdp.autofillAddress] = vdp.autofillData;
    vdp.autofillAddress.byte(0)++;
  }
}

auto M32X::vdpDMA() -> void {
  print("* 32X DMA\n");
  print(hex(dreq.source), " to ", hex(dreq.target), " for ", hex(dreq.length), "\n");
}
