auto Serial::serialize(serializer& s) -> void {
  s(state.dataTx);
  s(state.dataRx);
  s(state.baudRate);
  s(state.enable);
  s(state.rxOverrun);
  s(state.txFull);
  s(state.rxFull);
  s(state.baudClock);
  s(state.txBitClock);
  s(state.rxBitClock);
}
