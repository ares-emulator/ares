auto MemoryControl::serialize(serializer& s) -> void {
  s(ram.value);
  s(ram.delay);
  s(ram.window);

  s(cache.lock);
  s(cache.invalidate);
  s(cache.tagTest);
  s(cache.scratchpadEnable);
  s(cache.dataSize);
  s(cache.dataEnable);
  s(cache.codeSize);
  s(cache.codeEnable);
  s(cache.interruptPolarity);
  s(cache.readPriority);
  s(cache.noWaitState);
  s(cache.busGrant);
  s(cache.loadScheduling);
  s(cache.noStreaming);
  s(cache.reserved);

  s(common.com0);
  s(common.com1);
  s(common.com2);
  s(common.com3);
  s(common.unused);

  auto port = [&](MemPort& p) {
    s(p.writeDelay);
    s(p.readDelay);
    s(p.recovery);
    s(p.hold);
    s(p.floating);
    s(p.preStrobe);
    s(p.dataWidth);
    s(p.autoIncrement);
    s(p.unknown14_15);
    s(p.addrBits);
    s(p.reserved21_23);
    s(p.dmaTiming);
    s(p.addrError);
    s(p.dmaSelect);
    s(p.wideDMA);
    s(p.wait);
  };

  port(exp1);
  port(exp2);
  port(exp3);
  port(bios);
  port(spu);
  port(cdrom);
}
