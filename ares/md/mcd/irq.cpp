auto MCD::IRQ::raise() -> bool {
  if(pending) return false;
  pending = enable;
  synchronize();
  return true;
}

auto MCD::IRQ::lower() -> bool {
  if(!pending) return false;
  pending = 0;
  synchronize();
  return true;
}

auto MCD::IRQ::synchronize() -> void {
  mcd.irq.pending |= mcd.irq.reset.pending;
  mcd.irq.pending |= mcd.gpu.irq.pending;
  mcd.irq.pending |= mcd.external.irq.pending;
  mcd.irq.pending |= mcd.timer.irq.pending;
  mcd.irq.pending |= mcd.cdd.irq.pending;
  mcd.irq.pending |= mcd.cdc.irq.pending;
  mcd.irq.pending |= mcd.irq.subcode.pending;
}
