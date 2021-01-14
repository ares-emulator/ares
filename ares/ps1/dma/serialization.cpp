auto DMA::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(irq.force);
  s(irq.enable);
  s(irq.flag);
  s(irq.unknown);

  for(auto& channel : channels) {
    s(channel);
  }
  s(channelsByPriority);
}

auto DMA::Channel::serialize(serializer& s) -> void {
  s(masterEnable);
  s(priority);
  s(address);
  s(length);
  s(blocks);
  s(direction);
  s(decrement);
  s(synchronization);
  s(chopping.enable);
  s(chopping.dmaWindow);
  s(chopping.cpuWindow);
  s(enable);
  s(trigger);
  s(unknown);
  s(irq.enable);
  s(irq.flag);
  s(chain.length);
  s(chain.address);
  s(state);
  s(counter);
}
