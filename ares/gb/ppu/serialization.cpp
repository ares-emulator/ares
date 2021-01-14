auto PPU::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(vram);
  s(oam);
  s(bgp);
  s(obp);
  s(bgpd);
  s(obpd);

  s(status.irq);
  s(status.lx);

  s(status.bgEnable);
  s(status.obEnable);
  s(status.obSize);
  s(status.bgTilemapSelect);
  s(status.bgTiledataSelect);
  s(status.windowDisplayEnable);
  s(status.windowTilemapSelect);
  s(status.displayEnable);

  s(status.mode);
  s(status.interruptHblank);
  s(status.interruptVblank);
  s(status.interruptOAM);
  s(status.interruptLYC);

  s(status.scy);
  s(status.scx);

  s(status.ly);
  s(status.lyc);

  s(status.dmaBank);
  s(status.dmaActive);
  s(status.dmaClock);

  s(status.wy);
  s(status.wx);

  s(status.vramBank);

  s(status.bgpiIncrement);
  s(status.bgpi);

  s(status.obpiIncrement);
  s(status.obpi);

  s(latch.displayEnable);
  s(latch.windowDisplayEnable);
  s(latch.wx);
  s(latch.wy);

  s(history.mode);

  s(bg.color);
  s(bg.palette);
  s(bg.priority);

  s(ob.color);
  s(ob.palette);
  s(ob.priority);

  for(auto& o : sprite) {
    s(o.x);
    s(o.y);
    s(o.tile);
    s(o.attributes);
    s(o.tiledata);
  }
  s(sprites);

  s(background.attributes);
  s(background.tiledata);

  s(window.attributes);
  s(window.tiledata);
}
