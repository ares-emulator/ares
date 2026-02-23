//serialization.cpp
auto TIA::serialize(serializer& s) -> void {
  Thread::serialize(s);

  for(auto n : range(writeQueue.maxItems)) {
    s(writeQueue.items[n].active);
    s(writeQueue.items[n].address);
    s(writeQueue.items[n].data);
    s(writeQueue.items[n].delay);
  }

  s(io.vcounter);
  s(io.hcounter);
  s(io.hmoveTriggered);
  s(io.vsync);
  s(io.vblank);
  s(io.bgColor);
  s(io.p0Color);
  s(io.p1Color);
  s(io.fgColor);

  s(playfield.graphics);
  s(playfield.mirror);
  s(playfield.scoreMode);
  s(playfield.priority);

  for(auto n : range(2)) {
    s(player[n].graphics[0]);
    s(player[n].graphics[1]);
    s(player[n].reflect);
    s(player[n].size);
    s(player[n].offset);
    s(player[n].delay);
    s(player[n].counter);
    s(player[n].startCounter);
    s(player[n].pixelCounter);
    s(player[n].widthCounter);
    s(player[n].starting);
    s(player[n].output);
    s(player[n].copy);
  }

  for(auto n : range(2)) {
    s(missile[n].enable);
    s(missile[n].lockedToPlayer);
    s(missile[n].size);
    s(missile[n].offset);
    s(missile[n].counter);
    s(missile[n].startCounter);
    s(missile[n].pixelCounter);
    s(missile[n].widthCounter);
    s(missile[n].starting);
    s(missile[n].output);
  }

  s(ball.enable[0]);
  s(ball.enable[1]);
  s(ball.delay);
  s(ball.size);
  s(ball.offset);
  s(ball.counter);
  s(ball.output);

  s(collision.M0P0);
  s(collision.M0P1);
  s(collision.M1P0);
  s(collision.M1P1);
  s(collision.P0PF);
  s(collision.P0BL);
  s(collision.P1PF);
  s(collision.P1BL);
  s(collision.M0PF);
  s(collision.M0BL);
  s(collision.M1PF);
  s(collision.M1BL);
  s(collision.BLPF);
  s(collision.P0P1);
  s(collision.M0M1);

  for(auto n : range(26)) {
    s(volume[n]);
  }

  for(auto n : range(2)) {
    s(audio[n].enable);
    s(audio[n].divCounter);
    s(audio[n].noiseCounter);
    s(audio[n].noiseFeedback);
    s(audio[n].pulseCounter);
    s(audio[n].pulseCounterPaused);
    s(audio[n].pulseFeedback);
    s(audio[n].volume);
    s(audio[n].control);
    s(audio[n].frequency);
  }
}
