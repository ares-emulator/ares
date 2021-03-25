auto SH2::SCI::run() -> void {
  if(!link) return;  //no SH2 paired
  if(!link->sci.scr.re) return;  //receiver not enabled
  if(ssr.tdre) return;  //data not ready

  link->sci.rdr = tdr;
  link->sci.ssr.rdrf = 1;
  ssr.tdre = 1;

  if(scr.tie) {
    pendingTransmitEmptyIRQ = 1;
  }

  if(link->sci.scr.rie) {
    link->sci.pendingReceiveFullIRQ = 1;
  }
}
