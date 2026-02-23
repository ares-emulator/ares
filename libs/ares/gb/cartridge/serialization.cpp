auto Cartridge::serialize(serializer& s) -> void {
  Thread::serialize(s);
  s(bootromEnable);
  if(board) s(*board);
}
