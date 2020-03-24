auto Cartridge::serialize(serializer& s) -> void {
  board->serialize(s);
}
