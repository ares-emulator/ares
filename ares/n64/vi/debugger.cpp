auto VI::Debugger::load(Node::Object parent) -> void {
  framebuffer = parent->append<Node::Debugger::Graphics>("VI Framebuffer");
  updateGraphicsSize();
  framebuffer->setCapture([&]() -> std::vector<u32> {
    updateGraphicsSize();
    u32 width = framebuffer->width();
    u32 height = framebuffer->height();
    std::vector<u32> output(width * height, 0xff000000);
    if(vi.io.colorDepth != 2 && vi.io.colorDepth != 3) return output;

    u32 bytesPerPixel = vi.io.colorDepth == 2 ? 2 : 4;
    u32 base = vi.io.dramAddress;
    for(u32 y : range(height)) {
      for(u32 x : range(width)) {
        u64 address = (u64)base + ((u64)y * width + x) * bytesPerPixel;
        if(address + bytesPerPixel > rdram.ram.size) continue;
        if(vi.io.colorDepth == 2) {
          u16 pixel = rdram.ram.read<Half>(address, RBusDevice::ARES_DEBUGGER);
          u8 r = pixel >> 11 & 31; r = r << 3 | r >> 2;
          u8 g = pixel >>  6 & 31; g = g << 3 | g >> 2;
          u8 b = pixel >>  1 & 31; b = b << 3 | b >> 2;
          output[y * width + x] = 0xff000000 | r << 16 | g << 8 | b;
        } else {
          u32 pixel = rdram.ram.read<Word>(address, RBusDevice::ARES_DEBUGGER);
          output[y * width + x] = 0xff000000 | pixel >> 8;
        }
      }
    }
    return output;
  });

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "VI");
}

auto VI::Debugger::updateGraphicsSize() -> void {
  if(!framebuffer) return;
  u32 width = max(1u, (u32)vi.io.width);
  u32 lines = vi.io.vend >= vi.io.vstart ? vi.io.vend - vi.io.vstart : 0;
  u32 height = min(480u, (u32)(((u64)lines * vi.io.yscale + 2047) >> 11));
  framebuffer->setSize(width, max(1u, height));
}

auto VI::Debugger::io(bool mode, u32 address, u32 data) -> void {
  static const std::vector<string> registerNames = {
    "VI_CONTROL",
    "VI_DRAM_ADDRESS",
    "VI_H_WIDTH",
    "VI_V_INTR",
    "VI_V_CURRENT_LINE",
    "VI_TIMING",
    "VI_V_SYNC",
    "VI_H_SYNC",
    "VI_H_SYNC_LEAP",
    "VI_H_VIDEO",
    "VI_V_VIDEO",
    "VI_V_BURST",
    "VI_X_SCALE",
    "VI_Y_SCALE",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = (address < registerNames.size() ? registerNames[address] : string("VI_UNKNOWN"));
    if(mode == Read) {
      message = {nall::split(name, "|").front(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {nall::split(name, "|").back(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
