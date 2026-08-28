auto TIA::ObjectPipeline::decode(n3 mode, u8 counter) -> u8 {
  //Return the main or additional copy selected by the shared counter.
  if(counter == 156) return 1;
  if(counter == 12 && (mode == 1 || mode == 3)) return 2;
  if(counter == 28 && (mode == 2 || mode == 3 || mode == 6)) return mode == 3 ? 3 : 2;
  if(counter == 60 && (mode == 4 || mode == 6)) return mode == 6 ? 3 : 2;
  return 0;
}

auto TIA::ObjectPipeline::clock(n1 hblank, u8 hcounter) -> Signals {
  if(hblank) {
    ball.latchOutput();
    for(auto& pair : this->pair) pair.player.latchOutput();
    for(auto& pair : this->pair) pair.missile.latchOutput();
    return signals();
  }

  ball.clock();
  n1 resetMissile[2] = {};
  for(auto n : range(2)) resetMissile[n] = pair[n].player.clock();
  for(auto n : range(2)) {
    pair[n].missile.clock(1, 1, hcounter);
    if(resetMissile[n] && pair[n].missile.lockedToPlayer) {
      pair[n].missile.counter = pair[n].player.missileResetCounter();
    }
  }
  return signals();
}

auto TIA::ObjectPipeline::movementClock(u32 phase, n1 hblank, u8 hcounter) -> void {
  //A late HMOVE pulse reaches the object counters before the scanline rolls
  //over. Earlier visible pulses merge with the regular CLKP clock.
  auto lateHmove = !hblank && hcounter >= 220;

  for(auto& pair : this->pair) {
    auto& player = pair.player;
    if(!player.moving) continue;
    if(phase == (player.offset ^ 8)) {
      player.moving = 0;
    } else if((hblank || lateHmove) && player.clock() && pair.missile.lockedToPlayer) {
      //RESMP is owned by the player/missile pair.
      pair.missile.counter = player.missileResetCounter();
    }
  }

  for(auto& pair : this->pair) {
    auto& missile = pair.missile;
    if(!missile.moving) continue;
    if(phase == (missile.offset ^ 8)) missile.moving = 0;
    else if(hblank || lateHmove) missile.clock(1, 0, hcounter);
  }

  ball.lastMovementCounter = ball.counter;
  if(ball.moving) {
    if(phase == (ball.offset ^ 8)) ball.moving = 0;
    else if(hblank || lateHmove) ball.clock(1, 0);
  }
}

auto TIA::ObjectPipeline::runMovement(n1 hblank, u8 hcounter) -> void {
  if(!movementActive() || (hcounter & 3)) return;

  u32 clock = movementPhase > 15 ? 0 : (u32)movementPhase;
  movementClock(clock, hblank, hcounter);
  if(movementPhase < 16) movementPhase++;
}

auto TIA::ObjectPipeline::movementActive() const -> n1 {
  return player(0).moving || player(1).moving || missile(0).moving
    || missile(1).moving || ball.moving;
}

auto TIA::ObjectPipeline::hmove() -> void {
  movementPhase = 0;
  for(auto& pair : this->pair) pair.player.moving = 1;
  for(auto& pair : this->pair) pair.missile.moving = 1;
  ball.moving = 1;
}

auto TIA::ObjectPipeline::hmclr() -> void {
  for(auto& pair : this->pair) pair.player.offset = 0;
  for(auto& pair : this->pair) pair.missile.offset = 0;
  ball.offset = 0;
}

auto TIA::ObjectPipeline::signals() const -> Signals {
  Signals output;
  output.player[0] = pair[0].player.output;
  output.player[1] = pair[1].player.output;
  output.missile[0] = pair[0].missile.output;
  output.missile[1] = pair[1].missile.output;
  output.ball = ball.output;
  return output;
}

auto TIA::ObjectPipeline::nusiz(n1 index, n8 data, n1 hblank) -> void {
  pair[index].player.nusiz(data.bit(0, 2), hblank);
  pair[index].missile.nusiz(data);
}

auto TIA::ObjectPipeline::power() -> void {
  movementPhase = 0;
  for(auto& pair : this->pair) {
    pair = {};
    pair.player.divider = 1;
    pair.player.dividerPending = 1;
    pair.player.dividerChangeCounter = -1;
  }
  ball = {};
  ball.effectiveWidth = 1;
}
