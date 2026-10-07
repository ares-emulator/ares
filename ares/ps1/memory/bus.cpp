auto Bus::acquire(u8 owner) -> bool {
  if(owner == Idle) return false;
  if(arbiter.owner == owner) return true;
  if(arbiter.owner != Idle) {
    if(owner < DMA0 && arbiter.owner >= DMA0) arbiter.cpuPending |= 1 << owner;
    return false;
  }
  if(owner >= DMA0 && arbiter.cpuHandoff && arbiter.cpuPending
  && (dma.cpuControl & 7) <= dma.channels[owner - DMA0].priority) return false;
  if(owner < DMA0) {
    arbiter.cpuPending &= ~(1 << owner);
    arbiter.cpuHandoff = false;
  }
  arbiter.owner = owner;
  arbiter.grant = arbiter.sequence++;
  return true;
}

auto Bus::release(u8 owner, bool dmaBoundary) -> void {
  if(arbiter.owner != owner) return;
  arbiter.owner = Idle;
  if(owner >= DMA0 && dmaBoundary && arbiter.cpuPending) arbiter.cpuHandoff = true;
}

auto Bus::power() -> void {
  arbiter = {};
}

auto Bus::serialize(serializer& s) -> void {
  s(arbiter.owner);
  s(arbiter.sequence);
  s(arbiter.grant);
  s(arbiter.cpuPending);
  s(arbiter.cpuHandoff);
}
