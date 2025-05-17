auto PPI::serialize(serializer& s) -> void {
  I8255::serialize(s);
  s(io.inputSelect);
}
