auto PCD::serialize(serializer& s) -> void {
  Thread::serialize(s);
  wram.serialize(s);
  bram.serialize(s);
  scsi.serialize(s);
  adpcm.serialize(s);
  s.array(io.mdr);
}

auto PCD::BRAM::serialize(serializer& s) -> void {
  memory.serialize(s);
  s.integer(writable);
}

auto PCD::SCSI::serialize(serializer& s) -> void {
}

auto PCD::ADPCM::serialize(serializer& s) -> void {
  msm5205.serialize(s);
  memory.serialize(s);
}
