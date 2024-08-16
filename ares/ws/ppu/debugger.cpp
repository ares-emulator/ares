auto PPU::Debugger::load(Node::Object parent) -> void {
  graphics.screen1 = parent->append<Node::Debugger::Graphics>("Screen 1");
  graphics.screen1->setSize(256, 256);
  graphics.screen1->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(256 * 256);
    for(u32 y : range(256)) {
      for(u32 x : range(256)) {
        PPU::Output pixel;
        ppu.screen1.screenPixel(x, y, pixel);
        if(!pixel.valid) continue;
        u32 b = pixel.color.bit(0, 3);
        u32 g = pixel.color.bit(4, 7);
        u32 r = pixel.color.bit(8,11);
        output[y * 256 + x] = ((r * 0x11) << 16) | ((g * 0x11) << 8) | (b * 0x11);
      }
    }
    return output;
  });

  graphics.screen2 = parent->append<Node::Debugger::Graphics>("Screen 2");
  graphics.screen2->setSize(256, 256);
  graphics.screen2->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(256 * 256);
    for(u32 y : range(256)) {
      for(u32 x : range(256)) {
        PPU::Output pixel;
        ppu.screen2.screenPixel(x, y, pixel);
        if(!pixel.valid) continue;
        u32 b = pixel.color.bit(0, 3);
        u32 g = pixel.color.bit(4, 7);
        u32 r = pixel.color.bit(8,11);
        output[y * 256 + x] = ((r * 0x11) << 16) | ((g * 0x11) << 8) | (b * 0x11);
      }
    }
    return output;
  });


  graphics.tiles = parent->append<Node::Debugger::Graphics>("Tiles");
  graphics.tiles->setSize(256, 256);
  graphics.tiles->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(256 * 256);
    for(u32 tileY : range(32)) {
      if(tileY == 16 && !system.color()) break;
      for(u32 tileX : range(32)) {
        n16 address = 0x2000 + ((tileY * 32 + tileX) << 4);
        if(self.depth() == 4) {
          for(u32 y : range(8)) {
            n32 d0 = iram.read32((address + (y << 1)) << 1);
            for(u32 x : range(8)) {
              n4 color;
              if(self.packed()) {
                color.bit(0, 3) = bswap32(d0) >> ((7 - x) << 2);
              } else {
                color.bit(0) = d0.bit( 7 - x);
                color.bit(1) = d0.bit(15 - x);
                color.bit(2) = d0.bit(23 - x);
                color.bit(3) = d0.bit(31 - x);
              }
              output[(tileY * 8 + y) * 256 + (tileX * 8 + x)] = color * 0x111111;
            }
          }
        } else {
          for(u32 y : range(8)) {
            n16 d0 = iram.read16(address + (y << 1));
            for(u32 x : range(8)) {
              n2 color;
              color.bit(0) = d0.bit( 7 - x);
              color.bit(1) = d0.bit(15 - x);
              output[(tileY * 8 + y) * 256 + (tileX * 8 + x)] = color * 0x555555;
            }
          }
        }
      }
    }
    return output;
  });

  properties.ports = parent->append<Node::Debugger::Properties>("Display I/O");
  properties.ports->setQuery([&] { return ports(); });
}

auto PPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(properties.ports);
  parent->remove(graphics.tiles);
  parent->remove(graphics.screen2);
  parent->remove(graphics.screen1);
  properties.ports.reset();
  graphics.tiles.reset();
  graphics.screen2.reset();
  graphics.screen1.reset();
}

auto PPU::Debugger::ports() -> string {
  string output;
  output.append("Screen 1: ", hex(self.screen1.mapBase[1] << 11, 4L) , "; ", self.screen1.enable[1] ? "enabled" : "disabled", "\n");
  output.append("Screen 1 Scroll: ", self.screen1.hscroll[1], ", ", self.screen1.vscroll[1], "\n");
  output.append("Screen 1: ", hex(self.screen2.mapBase[1] << 11, 4L) , "; ", self.screen2.enable[1] ? "enabled" : "disabled", "\n");
  output.append("Screen 2 Scroll: ", self.screen2.hscroll[1], ", ", self.screen2.vscroll[1], "\n");
  output.append("Screen 2 Window: ",
    self.screen2.window.x0[1], ", ", self.screen2.window.y0[1], " -> ",
    self.screen2.window.x1[1], ", ", self.screen2.window.y1[1], "; ",
    self.screen2.window.enable[1] ? (self.screen2.window.invert[1] ? "inverted" : "enabled") : "disabled",
    "\n");
  output.append("Sprites: ", hex(self.sprite.oamBase << 9, 4L) , "; ", self.sprite.enable[1] ? "enabled" : "disabled", "\n");
  output.append("Sprite First: ", self.sprite.first, "\n");
  output.append("Sprite Count: ", self.sprite.count, "\n");
  output.append("Sprite Window: ",
    self.sprite.window.x0[1], ", ", self.sprite.window.y0[1], " -> ",
    self.sprite.window.x1[1], ", ", self.sprite.window.y1[1], "; ",
    self.sprite.window.enable[1] ? (self.sprite.window.invert[1] ? "inverted" : "enabled") : "disabled",
    "\n");
  
  output.append("Line Compare: ", self.io.vcounter, " == ", self.io.vcompare, "\n");

  output.append("Shade LUT: ");
  for(u32 i : range(8)) {
    if(i > 0) output.append(", ");
    output.append(self.pram.pool[i]);
  }
  for(u32 i : range(16)) {
    output.append("\n");
    output.append(i >= 8 ? "SPR" : "SCR", " Palette ", (i & 7), " (", i, "): ");
    for(u32 j : range(4)) {
      if(j > 0) output.append(", ");
      output.append(self.pram.palette[i].color[j], " (", self.pram.pool[self.pram.palette[i].color[j]], ")");
    }
  }

  output.append("\n");
  output.append("HBlank Timer: ", self.htimer.counter, "/", self.htimer.frequency, "; ", self.htimer.enable ? "enabled" : "disabled", ", ", self.htimer.repeat ? "repeat" : "one-shot", "\n");
  output.append("VBlank Timer: ", self.vtimer.counter, "/", self.vtimer.frequency, "; ", self.vtimer.enable ? "enabled" : "disabled", ", ", self.vtimer.repeat ? "repeat" : "one-shot", "\n");

  output.append("LCD Timing: ", (self.io.vtotal + 1), " lines/frame (", (12000.0 / (self.io.vtotal + 1)), " Hz), VBP @ line ", self.io.vsync);
  if(self.io.vtotal != self.io.vsync + 3) {
    output.append(" (unexpected)");
  }

  return output;
}
