auto Video::serialize(serializer& s) -> void {
  s((u32&)sync);
  s(lineCounter);
  s(linesSinceReturn);
  s(pulseLines);
}
