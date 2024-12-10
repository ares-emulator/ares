auto VI::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(io.colorDepth);
  s(io.gammaDither);
  s(io.gamma);
  s(io.divot);
  s(io.serrate);
  s(io.antialias);
  s(io.reserved);
  s(io.dramAddress);
  s(io.width);
  s(io.coincidence);
  s(io.hsyncWidth);
  s(io.colorBurstWidth);
  s(io.vsyncWidth);
  s(io.colorBurstHsync);
  s(io.halfLinesPerField);
  s(io.quarterLineDuration);
  s(io.leapPattern);
  s(io.hsyncLeap);
  s(io.hend);
  s(io.hstart);
  s(io.vend);
  s(io.vstart);
  s(io.colorBurstEnd);
  s(io.colorBurstStart);
  s(io.xscale);
  s(io.xsubpixel);
  s(io.yscale);
  s(io.ysubpixel);
  s(io.vcounter);
  s(io.field);
  s(io.leapCounter);

  s(clockFraction);
}
