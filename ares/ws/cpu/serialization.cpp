auto CPU::serialize(serializer& s) -> void {
  V30MZ::serialize(s);
  Thread::serialize(s);
  s(r.dmaSource);
  s(r.dmaTarget);
  s(r.dmaLength);
  s(r.dmaEnable);
  s(r.dmaMode);
  s(r.cartridgeEnable);
  s(r.interruptBase);
  s(r.serialData);
  s(r.interruptEnable);
  s(r.serialBaudRate);
  s(r.serialEnable);
  s(r.interruptStatus);
  s(r.keypadMatrix);
}
