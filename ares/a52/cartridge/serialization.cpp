auto Cartridge::serialize(serializer& s) -> void {
  if(board) s(*board);
}
