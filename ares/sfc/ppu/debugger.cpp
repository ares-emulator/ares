auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("PPU VRAM");
  memory.vram->setSize(self.vram.mask + 1 << 1);
  memory.vram->setRead([&](u32 address) -> u8 {
    return self.vram.data[address >> 1 & self.vram.mask].byte(address & 1);
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    self.vram.data[address >> 1 & self.vram.mask].byte(address & 1) = data;
  });

  memory.oam = parent->append<Node::Debugger::Memory>("PPU OAM");
  memory.oam->setSize(512 + 32);
  memory.oam->setRead([&](u32 address) -> u8 {
    return self.oam.read(address);
  });
  memory.oam->setWrite([&](u32 address, u8 data) -> void {
    return self.oam.write(address, data);
  });

  memory.cgram = parent->append<Node::Debugger::Memory>("PPU CGRAM");
  memory.cgram->setSize(256 << 1);
  memory.cgram->setRead([&](u32 address) -> u8 {
    return self.dac.cgram[address >> 1 & 255].byte(address & 1);
  });
  memory.cgram->setWrite([&](u32 address, u8 data) -> void {
    self.dac.cgram[address >> 1 & 255].byte(address & 1) = data;
  });

  graphics.tiles2bpp = parent->append<Node::Debugger::Graphics>("2 BPP Tiles");
  graphics.tiles2bpp->setSize(512, 512);
  graphics.tiles2bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(512 * 512);
    for(u32 tileY : range(64)) {
      for(u32 tileX : range(64)) {
        n15 address = tileY * 64 + tileX << 3;
        for(u32 y : range(8)) {
          n16 d0 = self.vram.data[address + y];
          for(u32 x : range(8)) {
            n2 color;
            color.bit(0) = d0.bit( 7 - x);
            color.bit(1) = d0.bit(15 - x);
            output[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color * 0x555555;
          }
        }
      }
    }
    return output;
  });

  graphics.tiles4bpp = parent->append<Node::Debugger::Graphics>("4 BPP Tiles");
  graphics.tiles4bpp->setSize(512, 256);
  graphics.tiles4bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(512 * 256);
    for(u32 tileY : range(32)) {
      for(u32 tileX : range(64)) {
        n15 address = tileY * 64 + tileX << 4;
        for(u32 y : range(8)) {
          n16 d0 = self.vram.data[address + y + 0];
          n16 d1 = self.vram.data[address + y + 8];
          for(u32 x : range(8)) {
            n4 color;
            color.bit(0) = d0.bit( 7 - x);
            color.bit(1) = d0.bit(15 - x);
            color.bit(2) = d1.bit( 7 - x);
            color.bit(3) = d1.bit(15 - x);
            output[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color * 0x111111;
          }
        }
      }
    }
    return output;
  });

  graphics.tiles8bpp = parent->append<Node::Debugger::Graphics>("8 BPP Tiles");
  graphics.tiles8bpp->setSize(512, 128);
  graphics.tiles8bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(512 * 128);
    for(u32 tileY : range(16)) {
      for(u32 tileX : range(64)) {
        n15 address = tileY * 64 + tileX << 5;
        for(u32 y : range(8)) {
          n16 d0 = self.vram.data[address + y +  0];
          n16 d1 = self.vram.data[address + y +  8];
          n16 d2 = self.vram.data[address + y + 16];
          n16 d3 = self.vram.data[address + y + 24];
          for(u32 x : range(8)) {
            n8 color;
            color.bit(0) = d0.bit( 7 - x);
            color.bit(1) = d0.bit(15 - x);
            color.bit(2) = d1.bit( 7 - x);
            color.bit(3) = d1.bit(15 - x);
            color.bit(4) = d2.bit( 7 - x);
            color.bit(5) = d2.bit(15 - x);
            color.bit(6) = d3.bit( 7 - x);
            color.bit(7) = d3.bit(15 - x);
            output[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color << 16 | color << 8 | color << 0;
          }
        }
      }
    }
    return output;
  });

  graphics.tilesMode7 = parent->append<Node::Debugger::Graphics>("Mode 7 Tiles");
  graphics.tilesMode7->setSize(128, 128);
  graphics.tilesMode7->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(128 * 128);
    for(u32 tileY : range(16)) {
      for(u32 tileX : range(16)) {
        n15 address = tileY * 16 + tileX << 6;
        for(u32 y : range(8)) {
          for(u32 x : range(8)) {
            n8 color = self.vram.data[address + y * 8 + x].byte(1);
            output[(tileY * 8 + y) * 128 + (tileX * 8 + x)] = color << 16 | color << 8 | color << 0;
          }
        }
      }
    }
    return output;
  });

  properties.registers = parent->append<Node::Debugger::Properties>("PPU Registers");
  properties.registers->setQuery([&] { return registers(); });
}

auto PPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.vram);
  parent->remove(memory.oam);
  parent->remove(memory.cgram);
  parent->remove(graphics.tiles2bpp);
  parent->remove(graphics.tiles4bpp);
  parent->remove(graphics.tiles8bpp);
  parent->remove(graphics.tilesMode7);
  parent->remove(properties.registers);
  memory.vram.reset();
  memory.oam.reset();
  memory.cgram.reset();
  graphics.tiles2bpp.reset();
  graphics.tiles4bpp.reset();
  graphics.tiles8bpp.reset();
  graphics.tilesMode7.reset();
  properties.registers.reset();
}

auto PPU::Debugger::registers() -> string {
  string output;
  output.append("Display Disable: ", self.io.displayDisable, "\n");
  output.append("Display Brightness: ", self.io.displayBrightness, "\n");
  output.append("BG Mode: ", self.io.bgMode, "\n");
  output.append("BG Priority: ", self.io.bgPriority, "\n");
  return output;
}
