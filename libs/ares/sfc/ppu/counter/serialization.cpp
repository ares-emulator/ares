auto PPUcounter::serialize(serializer& s) -> void {
  s(time.interlace);
  s(time.field);
  s(time.vperiod);
  s(time.hperiod);
  s(time.vcounter);
  s(time.hcounter);
  s(last.vperiod);
  s(last.hperiod);
}
