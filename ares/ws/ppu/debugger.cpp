auto PPU::Debugger::load(Node::Object parent) -> void {
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
                color.bit(0, 3) = d0.bit((7 - x) << 2);
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
}

auto PPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(graphics.tiles);
  graphics.tiles.reset();
}
