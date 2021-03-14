auto M32X::serialize(serializer& s) -> void {
  s(dram);
  s(sdram);
  s(shm);
  s(shs);
}

auto M32X::SHM::serialize(serializer& s) -> void {
  Thread::serialize(s);
}

auto M32X::SHS::serialize(serializer& s) -> void {
  Thread::serialize(s);
}
