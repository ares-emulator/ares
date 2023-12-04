auto SH2::INTC::run() -> void {
  if(self->inDelaySlot()) { self->cyclesUntilRecompilerExit = 0; return; }

  if(self->frt.pendingOutputIRQ) {
    if(self->SR.I < iprb.frtip) {
      self->interrupt(iprb.frtip, vcrc.focv);
      self->frt.pendingOutputIRQ = 0;
    }
  }

  if(self->dmac.pendingIRQ) {
    if(self->SR.I < ipra.dmacip) {
      if(self->dmac.pendingIRQ & 1) {
        self->interrupt(ipra.dmacip, self->dmac.vcrdma[0] & 0x7f);
        self->dmac.pendingIRQ &= ~1;
      } else if(self->dmac.pendingIRQ & 2) {
        self->interrupt(ipra.dmacip, self->dmac.vcrdma[1] & 0x7f);
        self->dmac.pendingIRQ &= ~2;
      }
    }
  }

  if(self->wdt.pendingIRQ) {
    if(self->SR.I < ipra.wdtip) {
      self->interrupt(ipra.wdtip, vcrwdt.witv);
      self->wdt.pendingIRQ = 0;
    }
  }

  if(self->sci.pendingTransmitEmptyIRQ) {
    if(self->SR.I < iprb.sciip) {
      self->interrupt(iprb.sciip, vcrb.stxv);
      self->sci.pendingTransmitEmptyIRQ = 0;
    }
  }

  if(self->sci.pendingReceiveFullIRQ) {
    if(self->SR.I < iprb.sciip) {
      self->interrupt(iprb.sciip, vcra.srxv);
      self->sci.pendingReceiveFullIRQ = 0;
    }
  }
}
