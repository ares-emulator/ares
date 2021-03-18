auto SH2::INTC::run() -> void {
  if(self->inDelaySlot()) return;
  if(self->frt.context.pendingOutputIRQ) {
    if(self->SR.I < iprb.frtip) {
      self->interrupt(iprb.frtip, vcrc.focv);
      self->frt.context.pendingOutputIRQ = 0;
    }
  }
}
