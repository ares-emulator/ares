auto SI::serialize(serializer& s) -> void {
  s(io.dramAddress);
  s(io.readAddress);
  s(io.writeAddress);
  s(io.busLatch);
  s(io.dmaBusy);
  s(io.ioBusy);
  s(io.readPending);
  s(io.pchState);
  s(io.dmaState);
  s(io.dmaError);
  s(io.interrupt);

  s(bbio.valid);
  for(auto channel : range(4)) {
    s(bbio.ch[channel].data);
    s(bbio.ch[channel].rxlen);
    s(bbio.ch[channel].txlen);
  }
}
