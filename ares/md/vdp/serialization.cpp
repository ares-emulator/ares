auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(dma);
  s(planeA);
  s(window);
  s(planeB);
  s(sprite);

  s(vram);
  s(vsram);
  s(cram);

  s(io.vblankIRQ);
  s(io.command);
  s(io.address);
  s(io.commandPending);
  s(io.displayOverlayEnable);
  s(io.counterLatch);
  s(io.horizontalBlankInterruptEnable);
  s(io.leftColumnBlank);
  s(io.videoMode);
  s(io.overscan);
  s(io.verticalBlankInterruptEnable);
  s(io.displayEnable);
  s(io.backgroundColor);
  s(io.horizontalInterruptCounter);
  s(io.externalInterruptEnable);
  s(io.displayWidth);
  s(io.interlaceMode);
  s(io.shadowHighlightEnable);
  s(io.externalColorEnable);
  s(io.horizontalSync);
  s(io.verticalSync);
  s(io.dataIncrement);

  s(latch.field);
  s(latch.interlace);
  s(latch.overscan);
  s(latch.horizontalInterruptCounter);
  s(latch.displayWidth);

  s(state.hdot);
  s(state.hcounter);
  s(state.vcounter);
  s(state.field);
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

  s(state.horizontalScroll);
  s(state.verticalScroll);

  s(output.color);
  s(output.priority);
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

  s(output.color);
  s(output.priority);

  for(u32 n : range(80)) s(oam[n]);
  for(u32 n : range(20)) s(objects[n]);
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
