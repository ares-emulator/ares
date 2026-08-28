auto TIA::Timing::rsync() -> void {
  auto displayPosition = position();
  hcounterDelta = 225 - displayPosition;
  hcounter = 225;
}

auto TIA::Timing::advance() -> bool {
  if(++hcounter < 228) return false;
  hcounter = 0;
  hcounterDelta = 0;
  return true;
}
