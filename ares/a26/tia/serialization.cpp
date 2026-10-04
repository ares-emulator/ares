//serialization.cpp
auto TIA::serialize(serializer& s) -> void {
  Thread::serialize(s);

  timing.serialize(s);
  for(auto& write : writes) write.serialize(s);
  s(vsync);
  s(vblank);
  objects.serialize(s);
  playfield.serialize(s);
  priority.serialize(s);
  collision.serialize(s);
  audio.serialize(s);
  triggers.serialize(s);
  analog.serialize(s);
}

auto TIA::Timing::serialize(serializer& s) -> void {
  s(hcounter);
  s(hcounterDelta);
  s(extendedHblank);
}

auto TIA::DelayedWrite::serialize(serializer& s) -> void {
  s(active);
  s(address);
  s(data);
  s(delay);
}

auto TIA::ObjectPipeline::Player::serialize(serializer& s) -> void {
  s(graphics[0]);
  s(graphics[1]);
  s(reflect);
  s(size);
  s(offset);
  s(moving);
  s(delay);
  s(counter);
  s(renderCounter);
  s(renderCounterTripPoint);
  s(sampleCounter);
  s(dividerChangeCounter);
  s(divider);
  s(dividerPending);
  s(rendering);
  s(output);
  s(copy);
}

auto TIA::ObjectPipeline::Missile::serialize(serializer& s) -> void {
  s(enable);
  s(lockedToPlayer);
  s(copies);
  s(size);
  s(offset);
  s(moving);
  s(counter);
  s(renderCounter);
  s(effectiveWidth);
  s(rendering);
  s(output);
  s(copy);
}

auto TIA::ObjectPipeline::Ball::serialize(serializer& s) -> void {
  s(enable[0]);
  s(enable[1]);
  s(delay);
  s(size);
  s(offset);
  s(moving);
  s(counter);
  s(renderCounter);
  s(effectiveWidth);
  s(lastMovementCounter);
  s(rendering);
  s(output);
}

auto TIA::ObjectPipeline::serialize(serializer& s) -> void {
  for(auto n : range(2)) player(n).serialize(s);
  for(auto n : range(2)) missile(n).serialize(s);
  ball.serialize(s);
  s(movementPhase);
}

auto TIA::Playfield::serialize(serializer& s) -> void {
  s(graphics);
  s(pixel);
  s(mirror);
  s(mirrorActive);
}

auto TIA::Priority::serialize(serializer& s) -> void {
  s(backgroundColor);
  s(playerColor[0]);
  s(playerColor[1]);
  s(playfieldColor);
  s(scoreMode);
  s(playfieldPriority);
}

auto TIA::Collision::serialize(serializer& s) -> void {
  s(M0P0);
  s(M0P1);
  s(M1P0);
  s(M1P1);
  s(P0PF);
  s(P0BL);
  s(P1PF);
  s(P1BL);
  s(M0PF);
  s(M0BL);
  s(M1PF);
  s(M1BL);
  s(BLPF);
  s(P0P1);
  s(M0M1);
}

auto TIA::Audio::Channel::serialize(serializer& s) -> void {
  s(enable);
  s(divCounter);
  s(noiseCounter);
  s(noiseFeedback);
  s(pulseCounter);
  s(pulseCounterPaused);
  s(pulseFeedback);
  s(volume);
  s(control);
  s(frequency);
}

auto TIA::Audio::serialize(serializer& s) -> void {
  for(auto& item : channel) item.serialize(s);
  s(phase);
  s(sum);
  s(clocks);
}

auto TIA::TriggerInputs::Input::serialize(serializer& s) -> void {
  s(mode);
  s(value);
}

auto TIA::TriggerInputs::serialize(serializer& s) -> void {
  for(auto& item : input) item.serialize(s);
}

auto TIA::AnalogInputs::Input::serialize(serializer& s) -> void {
  s(voltage);
  s(timestamp);
  s(connection.type);
  s(connection.resistance);
}

auto TIA::AnalogInputs::serialize(serializer& s) -> void {
  s(time);
  s(dumped);
  for(auto& item : input) item.serialize(s);
}
