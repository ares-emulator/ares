auto VDP::serialize(serializer& s) -> void {
  if(Model::MSX()) TMS9918::serialize(s);
  if(Model::MSX2()) V9938::serialize(s);
  Thread::serialize(s);
}
