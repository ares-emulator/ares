auto BSMemoryCartridge::serialize(serializer& s) -> void {
  if(ROM) return;

  Thread::serialize(s);

  s(memory);

  s(pin.writable);

  s(chip.vendor);
  s(chip.device);
  s(chip.serial);

  s(page.buffer[0]);
  s(page.buffer[1]);

  for(auto& block : blocks) {
    s(block.id);
    s(block.erased);
    s(block.locked);
    s(block.erasing);
    s(block.status.vppLow);
    s(block.status.queueFull);
    s(block.status.aborted);
    s(block.status.failed);
    s(block.status.locked);
    s(block.status.ready);
  }

  s(compatible.status.vppLow);
  s(compatible.status.writeFailed);
  s(compatible.status.eraseFailed);
  s(compatible.status.eraseSuspended);
  s(compatible.status.ready);

  s(global.status.page);
  s(global.status.pageReady);
  s(global.status.pageAvailable);
  s(global.status.queueFull);
  s(global.status.sleeping);
  s(global.status.failed);
  s(global.status.suspended);
  s(global.status.ready);

  s(mode);

  s(readyBusyMode);

  s(queue);
}

auto BSMemoryCartridge::Queue::serialize(serializer& s) -> void {
  s(history[0].valid);
  s(history[0].address);
  s(history[0].data);

  s(history[1].valid);
  s(history[1].address);
  s(history[1].data);

  s(history[2].valid);
  s(history[2].address);
  s(history[2].data);

  s(history[3].valid);
  s(history[3].address);
  s(history[3].data);
}
