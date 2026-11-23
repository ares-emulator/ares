auto uPD4701A::serialize(serializer& s) -> void {
  s(cs);
  s(xy);
  s(ul);
  s(resetX);
  s(resetY);
  s(x);
  s(y);
  s(startX);
  s(startY);
  s(latchX);
  s(latchY);
  s(switches);
  s(latchSwitches);
  s(cf);
}
