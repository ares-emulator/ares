auto VDP::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(vram);
  s(io.address);
  s(io.plane);
  s(io.scrollX);
  s(io.scrollY);
  s(io.blank);
  s(io.incrementRow);
  s(io.window);
  s(io.swapPlanes);
  s(vcounter);
}
