auto GPU::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("GPU VRAM");
  memory.vram->setSize(1_MiB);
  memory.vram->setRead([&](u32 address) -> u8 {
    return gpu.vram.readByte(address);
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    return gpu.vram.writeByte(address, data);
  });

  graphics.vram15bpp = parent->append<Node::Debugger::Graphics>("GPU VRAM 15bpp");
  graphics.vram15bpp->setSize(1024, 512);
  graphics.vram15bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(1024 * 512);
    for(u32 y : range(512)) {
      for(u32 x : range(1024)) {
        u16 pixel = gpu.vram.readHalf(y * 2048 + x * 2);
        u8 r = pixel >>  0 & 31; r = r << 3 | r >> 2;
        u8 g = pixel >>  5 & 31; g = g << 3 | g >> 2;
        u8 b = pixel >> 10 & 31; b = b << 3 | b >> 2;
        u8 a = 255;
        output[y * 1024 + x] = a << 24 | r << 16 | g << 8 | b << 0;
      }
    }
    return output;
  });

  graphics.vram24bpp = parent->append<Node::Debugger::Graphics>("GPU VRAM 24bpp");
  graphics.vram24bpp->setSize(682, 512);
  graphics.vram24bpp->setCapture([&]() -> vector<u32> {
    vector<u32> output;
    output.resize(682 * 512);
    for(u32 y : range(512)) {
      for(u32 x : range(682)) {
        u32 pixel = gpu.vram.readWordUnaligned(y * 2048 + x * 3);
        u8 r = pixel >>  0;
        u8 g = pixel >>  8;
        u8 b = pixel >> 16;
        u8 a = 255;
        output[y * 682 + x] = a << 24 | r << 16 | g << 8 | b << 0;
      }
    }
    return output;
  });
}
