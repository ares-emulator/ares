auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(dma);
  s(layers);
  s(window);
  s(layerA);
  s(layerB);
  s(sprite);

  s(fifo);
  s(vram);
  s(vsram);
  s(cram);

  s(command.latch);
  s(command.target);
  s(command.ready);
  s(command.pending);
  s(command.address);
  s(command.increment);

  s(io.vblankInterruptTriggered);
  s(io.displayOverlayEnable);
  s(io.counterLatch);
  s(io.hblankInterruptEnable);
  s(io.leftColumnBlank);
  s(io.videoMode);
  s(io.overscan);
  s(io.vblankInterruptEnable);
  s(io.displayEnable);
  s(io.backgroundColor);
  s(io.hblankInterruptCounter);
  s(io.externalInterruptEnable);
  s(io.displayWidth);
  s(io.interlaceMode);
  s(io.shadowHighlightEnable);
  s(io.externalColorEnable);
  s(io.hsync);
  s(io.vsync);
  s(io.clockSelect);

  s(latch.interlace);
  s(latch.overscan);
  s(latch.hblankInterruptCounter);
  s(latch.displayWidth);
  s(latch.clockSelect);

  s(state.hdot);
  s(state.hcounter);
  s(state.vcounter);
  s(state.ecounter);
  s(state.field);
  s(state.hsync);
  s(state.vsync);
}

auto VDP::Cache::serialize(serializer& s) -> void {
  s(reading);
  s(data);
  s(upper);
  s(lower);
}

auto VDP::Slot::serialize(serializer& s) -> void {
  s(target);
  s(address);
  s(data);
  s(upper);
  s(lower);
}

auto VDP::FIFO::serialize(serializer& s) -> void {
  s(cache);
  s(slots);
}

auto VDP::DMA::serialize(serializer& s) -> void {
  s(active);
  s(mode);
  s(source);
  s(length);
  s(data);
  s(wait);
  s(enable);
}

auto VDP::Layers::serialize(serializer& s) -> void {
  s(hscrollMode);
  s(hscrollAddress);
  s(vscrollMode);
  s(nametableWidth);
  s(nametableHeight);
}

auto VDP::Window::serialize(serializer& s) -> void {
  s(hoffset);
  s(hdirection);
  s(voffset);
  s(vdirection);
  s(nametableAddress);
}

auto VDP::Layer::serialize(serializer& s) -> void {
  s(hscroll);
  s(vscroll);
  s(generatorAddress);
  s(nametableAddress);
  s(output.color);
  s(output.priority);
  s(output.backdrop);
}

auto VDP::Object::serialize(serializer& s) -> void {
  s(x);
  s(y);
  s(tileWidth);
  s(tileHeight);
  s(hflip);
  s(vflip);
  s(palette);
  s(priority);
  s(address);
  s(link);
}

auto VDP::Sprite::serialize(serializer& s) -> void {
  s(generatorAddress);
  s(nametableAddress);
  s(collision);
  s(overflow);
  s(output.color);
  s(output.priority);
  s(output.backdrop);
  for(auto n : range(80)) s(oam[n]);
  for(auto n : range(20)) s(objects[n]);
}

auto VDP::VRAM::serialize(serializer& s) -> void {
  s(memory);
  s(mode);
}

auto VDP::VSRAM::serialize(serializer& s) -> void {
  s(memory);
}

auto VDP::CRAM::serialize(serializer& s) -> void {
  s(memory);
}
