auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(psg);
  s(irq);
  s(prefetch);
  s(fifo);
  s(dma);
  s(layers);
  s(window);
  s(layerA);
  s(layerB);
  s(sprite);
  s(dac);

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

  s(test.address);

  s(latch.interlace);
  s(latch.overscan);
  s(latch.displayWidth);
  s(latch.clockSelect);

  s(state.counterLatchValue);
  s(state.hcounter);
  s(state.vcounter);
  s(state.field);
  s(state.hblank);
  s(state.vblank);
  s(state.refreshing);
}

auto VDP::PSG::serialize(serializer& s) -> void {
  SN76489::serialize(s);
  Thread::serialize(s);

  s(test.volumeOverride);
  s(test.volumeChannel);
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

auto VDP::Slot::serialize(serializer& s) -> void {
  s(target);
  s(address);
  s(data);
  s(upper);
  s(lower);
}

auto VDP::Prefetch::serialize(serializer& s) -> void {
  s(slot);
}

auto VDP::FIFO::serialize(serializer& s) -> void {
  s(slots);
}

auto VDP::DMA::serialize(serializer& s) -> void {
  s(active);
  s(mode);
  s(source);
  s(length);
  s(data);
  s(wait);
  s(read);
  s(enable);
}

auto VDP::Pixel::serialize(serializer& s) -> void {
  s(color);
  s(priority);
  s(backdrop);
}

auto VDP::Layers::serialize(serializer& s) -> void {
  s(hscrollMode);
  s(hscrollAddress);
  s(vscrollMode);
  s(nametableWidth);
  s(nametableHeight);
}

auto VDP::Attributes::serialize(serializer& s) -> void {
  s(address);
  s(hmask);
  s(vmask);
  s(hscroll);
  s(vscroll);
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
  s(attributes);
  s(pixels);
  s(colors);
  s(extras);
  s(windowed);
  s(mappings);
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
  s(maskCheck);
  s(maskActive);
  s(patternIndex);
  s(patternSlice);
  s(patternCount);
  s(visible);
  s(visibleLink);
  s(visibleCount);
  s(visibleStop);
  s(test.disablePhase1);
  s(test.disablePhase2);
  s(test.disablePhase3);
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
  s(test.disableLayers);
  s(test.forceLayer);
}

auto VDP::VRAM::serialize(serializer& s) -> void {
  s(memory);
  s(size);
  s(mode);
  s(refreshing);
}

auto VDP::VSRAM::serialize(serializer& s) -> void {
  s(memory);
}

auto VDP::CRAM::serialize(serializer& s) -> void {
  s(memory);
}
