auto SI::serialize(serializer& s) -> void {
  SM5K::serialize(s);
  Thread::serialize(s);
  s(io.dramAddress);
  s(io.readAddress);
  s(io.writeAddress);
  s(io.dmaBusy);
  s(io.ioBusy);
  s(io.readPending);
  s(io.pchState);
  s(io.dmaState);
  s(io.dmaError);
  s(io.interrupt);
}
