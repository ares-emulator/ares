auto SI::dmaRead() -> void {
  if(system._BB()) {
    debug(unimplemented, "[SI::dmaRead] BBPlayer SI");
  } else {
    pif.dmaRead(io.readAddress, io.dramAddress);
  }
  io.dmaBusy = 0;
  io.pchState = 0;
  io.dmaState = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::SI);
}

auto SI::dmaWrite() -> void {
  if(system._BB()) {
    debug(unimplemented, "[SI::dmaWrite] BBPlayer SI");
  } else {
    pif.dmaWrite(io.writeAddress, io.dramAddress);
  }
  io.dmaBusy = 0;
  io.pchState = 0;
  io.dmaState = 0;
  io.interrupt = 1;
  mi.raise(MI::IRQ::SI);
}
