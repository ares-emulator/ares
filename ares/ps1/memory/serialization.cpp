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
}
