auto Display::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(io.vblank);
  s(io.hblank);
  s(io.vcoincidence);
  s(io.irqvblank);
  s(io.irqhblank);
  s(io.irqvcoincidence);
  s(io.vcompare);
  s(io.vcounter);

  s(videoCapture);
}
