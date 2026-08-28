auto TIA::Priority::resolveSource(n1 playfield, const ObjectSignals& signals) const -> Source {
  //SCORE shares PF with P0/M0 on the left and P1/M1 on the right.
  auto bl = signals.ball;
  auto p0 = signals.player[0];
  auto p1 = signals.player[1];
  auto m0 = signals.missile[0];
  auto m1 = signals.missile[1];

  if(playfieldPriority) {
    if(playfield) return Source::Playfield;
    if(bl)        return Source::Ball;
    if(p0 || m0)  return Source::Player0;
    if(p1 || m1)  return Source::Player1;
                  return Source::Background;
  }

  if(scoreMode) {
    if(p0 || m0)  return Source::Player0;
    if(playfield) return Source::Playfield;
    if(p1 || m1)  return Source::Player1;
    if(bl)        return Source::Ball;
                  return Source::Background;
  }

  if(p0 || m0)  return Source::Player0;
  if(p1 || m1)  return Source::Player1;
  if(playfield) return Source::Playfield;
  if(bl)        return Source::Ball;
                return Source::Background;
}

auto TIA::Priority::resolveColor(Source source, i16 x) const -> n7 {
  switch(source) {
  case Source::Playfield: return scoreMode && !playfieldPriority
                                ? playerColor[x >= 80]
                                : playfieldColor;
  case Source::Ball:      return playfieldColor;
  case Source::Player0:   return playerColor[0];
  case Source::Player1:   return playerColor[1];
  default:                return backgroundColor;
  }
}
