auto Aleck64::serialize(serializer& s) -> void {
  s(sdram);
  s(vram);
  s(pram);

  s(vdp.io.enable);
  s(dipSwitch[0]);
  s(dipSwitch[1]);
}
