auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);
  if(board) s(*board);
}
