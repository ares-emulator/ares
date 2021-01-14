auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(vce);
  s(vdc0); if(Model::SuperGrafx())
  s(vdc1); if(Model::SuperGrafx())
  s(vpc);

  s(io.hcounter);
  s(io.vcounter);
}

auto VCE::serialize(serializer& s) -> void {
  s(cram.memory);
  s(cram.address);

  s(io.clock);
  s(io.extraLine);
  s(io.grayscale);
}

auto VDC::serialize(serializer& s) -> void {
  s(vram.memory);
  s(vram.addressRead);
  s(vram.addressWrite);
  s(vram.addressIncrement);
  s(vram.dataRead);
  s(vram.dataWrite);

  s(satb.memory);

  s(irq.line);
  s(irq.collision.enable);
  s(irq.collision.pending);
  s(irq.overflow.enable);
  s(irq.overflow.pending);
  s(irq.coincidence.enable);
  s(irq.coincidence.pending);
  s(irq.vblank.enable);
  s(irq.vblank.pending);
  s(irq.transferVRAM.enable);
  s(irq.transferVRAM.pending);
  s(irq.transferSATB.enable);
  s(irq.transferSATB.pending);

  s(dma.sourceIncrementMode);
  s(dma.targetIncrementMode);
  s(dma.satbRepeat);
  s(dma.source);
  s(dma.target);
  s(dma.length);
  s(dma.satbSource);
  s(dma.vramActive);
  s(dma.satbActive);
  s(dma.satbPending);
  s(dma.satbOffset);

  s(timing.horizontalSyncWidth);
  s(timing.horizontalDisplayStart);
  s(timing.horizontalDisplayWidth);
  s(timing.horizontalDisplayEnd);
  s(timing.verticalSyncWidth);
  s(timing.verticalDisplayStart);
  s(timing.verticalDisplayWidth);
  s(timing.verticalDisplayEnd);
  s(timing.hstate);
  s(timing.vstate);
  s(timing.hoffset);
  s(timing.voffset);
  s(timing.coincidence);

  s(latch.horizontalSyncWidth);
  s(latch.horizontalDisplayStart);
  s(latch.horizontalDisplayWidth);
  s(latch.horizontalDisplayEnd);
  s(latch.verticalSyncWidth);
  s(latch.verticalDisplayStart);
  s(latch.verticalDisplayWidth);
  s(latch.verticalDisplayEnd);

  s(io.address);
  s(io.externalSync);
  s(io.displayOutput);
  s(io.dramRefresh);
  s(io.coincidence);

  s(background.enable);
  s(background.vramMode);
  s(background.characterMode);
  s(background.hscroll);
  s(background.vscroll);
  s(background.vcounter);
  s(background.width);
  s(background.height);
  s(background.hoffset);
  s(background.voffset);
  s(background.latch.vramMode);
  s(background.latch.characterMode);

  s(sprite.enable);
  s(sprite.vramMode);
  s(sprite.latch.vramMode);
}

auto VDC::Object::serialize(serializer& s) -> void {
  s(y);
  s(x);
  s(characterMode);
  s(pattern);
  s(palette);
  s(priority);
  s(width);
  s(height);
  s(hflip);
  s(vflip);
  s(first);
}

auto VPC::serialize(serializer& s) -> void {
  for(auto& setting : settings) {
    s(setting.enableVDC0);
    s(setting.enableVDC1);
    s(setting.priority);
  }
  s(window);
  s(select);
}
