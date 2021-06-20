auto CPU::serialize(serializer& s) -> void {
  V30MZ::serialize(s);
  Thread::serialize(s);
  s(dma.source);
  s(dma.target);
  s(dma.length);
  s(dma.enable);
  s(dma.direction);
  s(keypad.matrix);
  s(io.cartridgeEnable);
  s(io.interruptBase);
  s(io.interruptEnable);
  s(io.interruptStatus);
  s(io.serialData);
  s(io.serialBaudRate);
  s(io.serialEnable);
}
