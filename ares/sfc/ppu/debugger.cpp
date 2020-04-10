auto PPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Memory>("PPU VRAM");
  memory.vram->setSize(ppu.vram.mask + 1 << 1);
  memory.vram->setRead([&](uint32 address) -> uint8 {
    return ppu.vram.data[address >> 1 & ppu.vram.mask].byte(address & 1);
  });
  memory.vram->setWrite([&](uint32 address, uint8 data) -> void {
    ppu.vram.data[address >> 1 & ppu.vram.mask].byte(address & 1) = data;
  });

  memory.oam = parent->append<Node::Memory>("PPU OAM");
  memory.oam->setSize(512 + 32);
  memory.oam->setRead([&](uint32 address) -> uint8 {
    return ppu.obj.oam.read(address);
  });
  memory.oam->setWrite([&](uint32 address, uint8 data) -> void {
    return ppu.obj.oam.write(address, data);
  });

  memory.cgram = parent->append<Node::Memory>("PPU CGRAM");
  memory.cgram->setSize(256 << 1);
  memory.cgram->setRead([&](uint32 address) -> uint8 {
    return ppu.dac.cgram[address >> 1 & 255].byte(address & 1);
  });
  memory.cgram->setWrite([&](uint32 address, uint8 data) -> void {
    ppu.dac.cgram[address >> 1 & 255].byte(address & 1) = data;
  });

  graphics.tiles2bpp = parent->append<Node::Graphics>("2 BPP Tiles");
  graphics.tiles2bpp->setSize(512, 512);
  graphics.tiles2bpp->setCapture([&]() -> vector<uint32_t> {
    vector<uint32_t> output;
    output.resize(512 * 512);
    for(uint tileY : range(64)) {
      for(uint tileX : range(64)) {
        uint15 address = tileY * 64 + tileX << 3;
        for(uint y : range(8)) {
          uint16 d0 = ppu.vram.data[address + y];
          for(uint x : range(8)) {
            uint2 color;
            color.bit(0) = d0.bit( 7 - x);
            color.bit(1) = d0.bit(15 - x);
            output[(tileY * 8 + y) * 512 + (tileX * 8 + x)] = color * 0x555555;
          }
        }
      }
    }
    return output;
  });

  graphics.tiles4bpp = parent->append<Node::Graphics>("4 BPP Tiles");
  graphics.tiles4bpp->setSize(512, 256);
  graphics.tiles4bpp->setCapture([&]() -> vector<uint32_t> {
    vector<uint32_t> output;
    output.resize(512 * 256);
    for(uint tileY : range(32)) {
      for(uint tileX : range(64)) {
        uint15 address = tileY * 64 + tileX << 4;
        for(uint y : range(8)) {
          uint16 d0 = ppu.vram.data[address + y + 0];
          uint16 d1 = ppu.vram.data[address + y + 8];
          for(uint x : range(8)) {
            uint4 color;
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

  graphics.tiles8bpp = parent->append<Node::Graphics>("8 BPP Tiles");
  graphics.tiles8bpp->setSize(512, 128);
  graphics.tiles8bpp->setCapture([&]() -> vector<uint32_t> {
    vector<uint32_t> output;
    output.resize(512 * 128);
    for(uint tileY : range(16)) {
      for(uint tileX : range(64)) {
        uint15 address = tileY * 64 + tileX << 5;
        for(uint y : range(8)) {
          uint16 d0 = ppu.vram.data[address + y +  0];
          uint16 d1 = ppu.vram.data[address + y +  8];
          uint16 d2 = ppu.vram.data[address + y + 16];
          uint16 d3 = ppu.vram.data[address + y + 24];
          for(uint x : range(8)) {
            uint8 color;
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

  graphics.tilesMode7 = parent->append<Node::Graphics>("Mode 7 Tiles");
  graphics.tilesMode7->setSize(128, 128);
  graphics.tilesMode7->setCapture([&]() -> vector<uint32_t> {
    vector<uint32_t> output;
    output.resize(128 * 128);
    for(uint tileY : range(16)) {
      for(uint tileX : range(16)) {
        uint15 address = tileY * 16 + tileX << 6;
        for(uint y : range(8)) {
          for(uint x : range(8)) {
            uint8 color = ppu.vram.data[address + y * 8 + x].byte(1);
            output[(tileY * 8 + y) * 128 + (tileX * 8 + x)] = color << 16 | color << 8 | color << 0;
          }
        }
      }
    }
    return output;
  });

  properties.registers = parent->append<Node::Properties>("PPU Registers");
  properties.registers->setQuery([&] { return registers(); });
}

auto PPU::Debugger::registers() -> string {
  string output;
  output.append("Display Disable: ", ppu.io.displayDisable, "\n");
  output.append("Display Brightness: ", ppu.io.displayBrightness, "\n");
  output.append("BG Mode: ", ppu.io.bgMode, "\n");
  output.append("BG Priority: ", ppu.io.bgPriority, "\n");
  return output;
}
