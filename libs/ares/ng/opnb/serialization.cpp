auto OPNB::serialize(serializer& s) -> void {
  s(busyCyclesRemaining);
  s(timerCyclesRemaining[0]);
  s(timerCyclesRemaining[1]);
  s(clocksPerSample);

  std::vector<uint8_t> buffer; // ymfm requires std::vector
  //regardless of whether we're reading or writing, 'save' first to get the buffer size
  ymfm::ymfm_saved_state writer(buffer, true);
  ym2610.save_restore(writer);

  for(auto& byte : buffer)s(byte);

  if(s.reading()) {
    ymfm::ymfm_saved_state state(buffer, false);
    ym2610.save_restore(state);
  }

  Thread::serialize(s);
}
