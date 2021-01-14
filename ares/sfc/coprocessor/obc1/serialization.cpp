auto OBC1::serialize(serializer& s) -> void {
  s(ram);
  s(status.address);
  s(status.baseptr);
  s(status.shift);
}
