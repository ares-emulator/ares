auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(irq);
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

  s(io.displayOverlayEnable);
  s(io.counterLatch);
  s(io.leftColumnBlank);
  s(io.videoMode);
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
  s(io.debugAddress);
  s(io.debugDisableLayers);
  s(io.debugForceLayer);
  s(io.debugDisableSpritePhase1);
  s(io.debugDisableSpritePhase2);
  s(io.debugDisableSpritePhase3);

  s(latch.interlace);
  s(latch.overscan);
  s(latch.displayWidth);
  s(latch.clockSelect);

  s(state.hcounter);
  s(state.vcounter);
  s(state.field);
  s(state.hblank);
  s(state.vblank);
  s(state.hclock);
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
}

auto VDP::Cache::serialize(serializer& s) -> void {
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

auto VDP::Pixel::serialize(serializer& s) -> void {
  s(color);
  s(priority);
}

auto VDP::Layers::serialize(serializer& s) -> void {
  s(hscrollMode);
  s(hscrollAddress);
  s(vscrollMode);
  s(nametableWidth);
  s(nametableHeight);
  s(vscrollIndex);
}

auto VDP::Attributes::serialize(serializer& s) -> void {
  s(address);
  s(width);
  s(height);
  s(hscroll);
  s(vscroll);
}

auto VDP::Window::serialize(serializer& s) -> void {
  s(hoffset);
  s(hdirection);
  s(voffset);
  s(vdirection);
  s(nametableAddress);
  s(attributesIndex);
}

auto VDP::Layer::serialize(serializer& s) -> void {
  s(hscroll);
  s(vscroll);
  s(generatorAddress);
  s(nametableAddress);
  s(attributes);
  s(pixels);
  s(colors);
  s(extras);
  s(windowed);
  s(mappings);
  s(mappingIndex);
  s(patternIndex);
  s(pixelCount);
  s(pixelIndex);
}

auto VDP::Layer::Mapping::serialize(serializer& s) -> void {
  s(address);
  s(hflip);
  s(palette);
  s(priority);
}

auto VDP::Sprite::serialize(serializer& s) -> void {
  s(generatorAddress);
  s(nametableAddress);
  s(collision);
  s(overflow);
  s(pixels);
  s(cache);
  s(mappings);
  s(mappingCount);
  s(patternX);
  s(patternIndex);
  s(patternSlice);
  s(patternCount);
  s(patternStop);
  s(visible);
  s(visibleLink);
  s(visibleCount);
  s(visibleStop);
  s(pixelIndex);
  s(vcounter);
}

auto VDP::Sprite::Cache::serialize(serializer& s) -> void {
  s(y);
  s(link);
  s(height);
  s(width);
}

auto VDP::Sprite::Mapping::serialize(serializer& s) -> void {
  s(valid);
  s(width);
  s(height);
  s(address);
  s(hflip);
  s(palette);
  s(priority);
  s(x);
}

auto VDP::DAC::serialize(serializer& s) -> void {
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
