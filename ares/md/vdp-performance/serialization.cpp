auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(vram);
  s(vsram);
  s(cram);
  s(psg);
  s(irq);
  s(dma);
  s(planeA);
  s(window);
  s(planeB);
  s(sprite);

  s(command.latch);
  s(command.target);
  s(command.ready);
  s(command.pending);
  s(command.address);
  s(command.increment);

  s(io.displayOverlayEnable);
  s(io.counterLatch);
  s(io.videoMode4);
  s(io.leftColumnBlank);
  s(io.videoMode5);
  s(io.overscan);
  s(io.displayEnable);
  s(io.backgroundColor);
  s(io.displayWidth);
  s(io.interlaceMode);
  s(io.shadowHighlightEnable);
  s(io.externalColorEnable);
  s(io.hsync);
  s(io.vsync);
  s(io.clockSelect);

  s(latch.interlace);
  s(latch.overscan);
  s(latch.displayWidth);
  s(latch.clockSelect);

  s(state.hcounter);
  s(state.vcounter);
  s(state.field);
  s(state.hblank);
  s(state.vblank);
}

auto VDP::PSG::serialize(serializer& s) -> void {
  SN76489::serialize(s);
  Thread::serialize(s);
}

auto VDP::IRQ::serialize(serializer& s) -> void {
  s(external.enable);
  s(external.pending);
  s(hblank.enable);
  s(hblank.pending);
  s(hblank.counter);
  s(hblank.frequency);
  s(vblank.enable);
  s(vblank.pending);
  s(vblank.transitioned);
}

auto VDP::VRAM::serialize(serializer& s) -> void {
  s(pixels);
  s(memory);
  s(size);
  s(mode);
}

auto VDP::VSRAM::serialize(serializer& s) -> void {
  s(memory);
}

auto VDP::CRAM::serialize(serializer& s) -> void {
  s(memory);
}

auto VDP::DMA::serialize(serializer& s) -> void {
  s(active);
  s(io.mode);
  s(io.source);
  s(io.length);
  s(io.fill);
  s(io.enable);
  s(io.wait);
}

auto VDP::Background::serialize(serializer& s) -> void {
  s(io.generatorAddress);
  s(io.nametableAddress);
  s(io.nametableWidth);
  s(io.nametableHeight);
  s(io.horizontalScrollAddress);
  s(io.horizontalScrollMode);
  s(io.verticalScrollMode);
  s(io.horizontalOffset);
  s(io.horizontalDirection);
  s(io.verticalOffset);
  s(io.verticalDirection);
}

auto VDP::Object::serialize(serializer& s) -> void {
  s(x);
  s(y);
  s(tileWidth);
  s(tileHeight);
  s(horizontalFlip);
  s(verticalFlip);
  s(palette);
  s(priority);
  s(address);
  s(link);
}

auto VDP::Sprite::serialize(serializer& s) -> void {
  s(io.generatorAddress);
  s(io.nametableAddress);
  for(auto& object : oam) s(object);
  for(auto& object : objects) s(object);
}
