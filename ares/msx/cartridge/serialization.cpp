auto Cartridge::serialize(serializer& s) -> void {
  if(board) board->serialize(s);
}
