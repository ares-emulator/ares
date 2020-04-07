auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);
  board->serialize(s);
}
