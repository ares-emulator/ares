auto RDP::Debugger::load(Node::Object parent) -> void {
  capture = std::make_unique<Capture>();
  cachedCommands.resize(80);
  frame = parent->append<Node::Debugger::GraphicsFrame>("RDP Frame");
  frame->setRequestCapture([&] { requestCapture(); });
  frame->setCaptureValid([&] { return captureReady(); });
  frame->setCommandCount([&] { return captureCommandCount(); });
  frame->setCommandText([&](u32 index) { return captureCommandText(index); });
  frame->setCommandArguments([&](u32 index) { return captureCommandArguments(index); });
  frame->setCommandDetail([&](u32 index) { return captureCommandDetail(index); });
  frame->setCommandOpcode([&](u32 index) { return captureCommandOpcode(index); });
  frame->setCommandType([&](u32 index) { return captureCommandType(index); });
  frame->setViews([&] { return captureViews(); });
  frame->setTicks([&] { return captureTicks(); });
  frame->setRender([&](u32 command, u32 view) { return renderCapture(command, view); });
  frame->setState([&](u32 command) { return captureStateText(command); });
  frame->setSummary([&] { return captureSummary(); });

  graphics.color = parent->append<Node::Debugger::Graphics>("RDP Color Image");
  graphics.depth = parent->append<Node::Debugger::Graphics>("RDP Depth Buffer");
  graphics.coverage = parent->append<Node::Debugger::Graphics>("RDP Coverage");
  updateGraphicsSize();

  auto imageCapture = [&](u32 view) -> std::vector<u32> {
    #if defined(VULKAN)
    vulkan.synchronize();
    #endif
    updateGraphicsSize();
    u32 width = graphics.color->width();
    u32 height = graphics.color->height();
    std::vector<u32> output(width * height, 0xff000000);
    u32 base = view == 1 ? rdp.set.mask.dramAddress : rdp.set.color.dramAddress;
    //The views are cropped to the scissor rectangle, but the buffer's row stride is
    //still the full color image width.
    u32 stride = max(1u, (u32)rdp.set.color.width + 1);
    auto region = rdpImageRegion(
      stride, rdp.scissor.x.hi, rdp.scissor.y.hi,
      rdp.scissor.x.lo, rdp.scissor.y.lo
    );
    u32 sourceX = region.x;
    u32 sourceY = region.y;
    u32 minimumDepth = 0x3ffff;
    u32 maximumDepth = 0;
    if(view == 1) {
      for(u32 y : range(height)) {
        for(u32 x : range(width)) {
          u64 sourceIndex = (u64)(y + sourceY) * stride + (x + sourceX);
          u64 address = base + sourceIndex * 2;
          if(address + 2 > rdram.ram.size) continue;
          u16 encoded = rdram.ram.read<Half>(address, RBusDevice::ARES_DEBUGGER) >> 2;
          if(encoded == 0x3fff) continue;  //cleared far plane
          u32 depth = rdpDecompressDepth(encoded);
          minimumDepth = min(minimumDepth, depth);
          maximumDepth = max(maximumDepth, depth);
        }
      }
    }

    for(u32 y : range(height)) {
      for(u32 x : range(width)) {
        u64 index = (u64)y * width + x;
        u64 sourceIndex = (u64)(y + sourceY) * stride + (x + sourceX);
        if(view == 0) {
          u32 format = rdp.set.color.format;
          u32 size = rdp.set.color.size;
          if(size == 2) {
            u64 address = base + sourceIndex * 2;
            if(address + 2 > rdram.ram.size || format != 0) continue;
            u16 pixel = rdram.ram.read<Half>(address, RBusDevice::ARES_DEBUGGER);
            output[index] = rdpDecodeRGBA16(pixel);
          } else if(size == 3) {
            u64 address = base + sourceIndex * 4;
            if(address + 4 > rdram.ram.size || format != 0) continue;
            output[index] = 0xff000000
              | rdram.ram.read<Word>(address, RBusDevice::ARES_DEBUGGER) >> 8;
          } else if(size == 1 && (format == 2 || format == 4)) {
            u64 address = base + sourceIndex;
            if(address >= rdram.ram.size) continue;
            u8 intensity = rdram.ram.read<Byte>(address, RBusDevice::ARES_DEBUGGER);
            output[index] = rdpDecodeGray(intensity);
          }
        } else if(view == 1) {
          u64 address = base + sourceIndex * 2;
          if(address + 2 > rdram.ram.size) continue;
          //RDP Z is a 14-bit inverted floating-point value; the low two bits
          //belong to compressed delta-Z.
          u16 encoded = rdram.ram.read<Half>(address, RBusDevice::ARES_DEBUGGER) >> 2;
          u8 intensity = 0;
          if(encoded != 0x3fff) {
            u32 depth = rdpDecompressDepth(encoded);
            u32 range = maximumDepth - minimumDepth;
            intensity = range ? 255 - (depth - minimumDepth) * 223 / range : 255;
          }
          output[index] = rdpDecodeGray(intensity);
        } else {
          u64 address = (base + sourceIndex * 2) >> 1;
          if(!rdram.hidden.data || address >= rdram.ram.size / 2) continue;
          u8 coverage = 7;
          if(rdp.set.color.size == 2) {
            u64 colorAddress = base + sourceIndex * 2;
            if(colorAddress + 2 > rdram.ram.size) continue;
            u16 color = rdram.ram.read<Half>(colorAddress, RBusDevice::ARES_DEBUGGER);
            coverage = (color & 1) << 2 | (rdram.hidden.data[address] & 3);
          } else if(rdp.set.color.size == 3) {
            u64 colorAddress = base + sourceIndex * 4;
            if(colorAddress + 4 > rdram.ram.size) continue;
            u32 color = rdram.ram.read<Word>(colorAddress, RBusDevice::ARES_DEBUGGER);
            coverage = (color & 0xff) >> 5;
          }
          u8 intensity = coverage * 255 / 7;
          output[index] = rdpDecodeGray(intensity);
        }
      }
    }
    return output;
  };
  graphics.color->setCapture([=] { return imageCapture(0); });
  graphics.depth->setCapture([=] { return imageCapture(1); });
  graphics.coverage->setCapture([=] { return imageCapture(2); });

  //No fixed-format TMEM image views: TMEM has no inherent width, format or origin —
  //only a tile descriptor gives it one, so the frame debugger's Tiles view is the only
  //place it can be decoded correctly. The hex view below stays; it makes no such claim.
  tmem = parent->append<Node::Debugger::Memory>("RDP TMEM");
  tmem->setSize(4_KiB);
  tmem->setRead([&](u32 address) -> u8 {
    #if defined(VULKAN)
    return vulkan.readTMEM(address);
    #else
    return 0;
    #endif
  });

  tracer.command = parent->append<Node::Debugger::Tracer::Notification>("Command", "RDP");
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "RDP");
}

auto RDP::Debugger::updateGraphicsSize() -> void {
  if(!graphics.color) return;
  //The struct names identify the high and low command-word halves, not the
  //numeric ordering of the coordinates. The first endpoint is normally smaller.
  u32 stride = max(1u, (u32)rdp.set.color.width + 1);
  auto region = rdpImageRegion(
    stride, rdp.scissor.x.hi, rdp.scissor.y.hi,
    rdp.scissor.x.lo, rdp.scissor.y.lo
  );
  graphics.color->setSize(region.width, region.height);
  graphics.depth->setSize(region.width, region.height);
  graphics.coverage->setSize(region.width, region.height);
}

//The Vulkan renderer consumes the command list before the software decoder sees it.
//Shadow the small amount of state needed by the live graphics nodes here.
auto RDP::Debugger::observeCommand(u32 code, const u32* words, u32 wordCount) -> void {
  u32 cacheIndex = code;
  bool cache = (code >= 0x2a && code <= 0x2f) || (code >= 0x37 && code <= 0x3f);
  if(code == 0x32) cache = true, cacheIndex = 64 + (words[1] >> 24 & 7);
  if(code == 0x35) cache = true, cacheIndex = 72 + (words[1] >> 24 & 7);
  if(cache) {
    auto& command = cachedCommands[cacheIndex];
    command.sequence = ++cachedCommandSequence;
    command.words.assign(words, words + wordCount);
  }

  if(code == 0x2d) {
    rdp.scissor.x.hi = words[0] >> 12 & 0xfff;
    rdp.scissor.y.hi = words[0] >>  0 & 0xfff;
    rdp.scissor.field = words[1] >> 25 & 1;
    rdp.scissor.odd = words[1] >> 24 & 1;
    rdp.scissor.x.lo = words[1] >> 12 & 0xfff;
    rdp.scissor.y.lo = words[1] >>  0 & 0xfff;
    updateGraphicsSize();
  }
  if(code == 0x3e) {
    rdp.set.mask.dramAddress = words[1] & 0x03ff'ffff;
  }
  if(code == 0x3f) {
    rdp.set.color.format = words[0] >> 21 & 7;
    rdp.set.color.size = words[0] >> 19 & 3;
    rdp.set.color.width = words[0] & 1023;
    rdp.set.color.dramAddress = words[1] & 0x03ff'ffff;
    updateGraphicsSize();
  }
}

auto RDP::Debugger::command(string_view message) -> void {
  if(unlikely(tracer.command->enabled())) {
    tracer.command->notify(message);
  }
}

auto RDP::Debugger::ioDPC(bool mode, u32 address, u32 data) -> void {
  static const std::vector<string> registerNames = {
    "DPC_START",
    "DPC_END",
    "DPC_CURRENT",
    "DPC_STATUS",
    "DPC_CLOCK",
    "DPC_BUSY",
    "DPC_PIPE_BUSY",
    "DPC_TMEM_BUSY",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = (address < registerNames.size() ? registerNames[address] : string("DPC_UNKNOWN"));
    if(mode == Read) {
      message = {nall::split(name, "|").front(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {nall::split(name, "|").back(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto RDP::Debugger::ioDPS(bool mode, u32 address, u32 data) -> void {
  static const std::vector<string> registerNames = {
    "DPS_TBIST",
    "DPS_TEST_MODE",
    "DPS_BUFTEST_ADDR",
    "DPS_BUFTEST_DATA",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = (address < registerNames.size() ? registerNames[address] : string("DPS_UNKNOWN"));
    if(mode == Read) {
      message = {nall::split(name, "|").front(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {nall::split(name, "|").back(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
