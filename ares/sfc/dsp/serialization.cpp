auto DSP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s.array(apuram);
  s.array(registers);

  s.integer(clock.counter);
  s.integer(clock.sample);

  s.integer(master.reset);
  s.integer(master.mute);
  s.array(master.volume);
  s.array(master.output);

  s.integer(echo.feedback);
  s.array(echo.volume);
  s.array(echo.fir);
  s.array(echo.history[0]);
  s.array(echo.history[1]);
  s.integer(echo.bank);
  s.integer(echo.delay);
  s.integer(echo.readonly);
  s.array(echo.input);
  s.array(echo.output);
  s.integer(echo._bank);
  s.integer(echo._readonly);
  s.integer(echo._address);
  s.integer(echo._offset);
  s.integer(echo._length);
  s.integer(echo._historyOffset);

  s.integer(noise.frequency);
  s.integer(noise.lfsr);

  s.integer(brr.bank);
  s.integer(brr._bank);
  s.integer(brr._source);
  s.integer(brr._address);
  s.integer(brr._nextAddress);
  s.integer(brr._header);
  s.integer(brr._byte);

  s.integer(latch.adsr0);
  s.integer(latch.envx);
  s.integer(latch.outx);
  s.integer(latch.pitch);
  s.integer(latch.output);

  for(auto& v : voice) v.serialize(s);
}

auto DSP::Voice::serialize(serializer& s) -> void {
  s.integer(index);

  s.array(volume);
  s.integer(pitch);
  s.integer(source);
  s.integer(adsr0);
  s.integer(adsr1);
  s.integer(gain);
  s.integer(envx);
  s.integer(keyon);
  s.integer(keyoff);
  s.integer(modulate);
  s.integer(noise);
  s.integer(echo);
  s.integer(end);

  s.array(buffer);
  s.integer(bufferOffset);
  s.integer(gaussianOffset);
  s.integer(brrAddress);
  s.integer(brrOffset);
  s.integer(keyonDelay);
  s.integer(envelopeMode);
  s.integer(envelope);

  s.integer(_envelope);
  s.integer(_keylatch);
  s.integer(_keyon);
  s.integer(_keyoff);
  s.integer(_modulate);
  s.integer(_noise);
  s.integer(_echo);
  s.integer(_end);
  s.integer(_looped);
}
