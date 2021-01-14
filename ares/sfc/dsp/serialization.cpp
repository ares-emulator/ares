auto DSP::serialize(serializer& s) -> void {
  Thread::serialize(s);

  s(apuram);
  s(registers);

  s(clock.counter);
  s(clock.sample);

  s(master.reset);
  s(master.mute);
  s(master.volume);
  s(master.output);

  s(echo.feedback);
  s(echo.volume);
  s(echo.fir);
  s(echo.history[0]);
  s(echo.history[1]);
  s(echo.bank);
  s(echo.delay);
  s(echo.readonly);
  s(echo.input);
  s(echo.output);
  s(echo._bank);
  s(echo._readonly);
  s(echo._address);
  s(echo._offset);
  s(echo._length);
  s(echo._historyOffset);

  s(noise.frequency);
  s(noise.lfsr);

  s(brr.bank);
  s(brr._bank);
  s(brr._source);
  s(brr._address);
  s(brr._nextAddress);
  s(brr._header);
  s(brr._byte);

  s(latch.adsr0);
  s(latch.envx);
  s(latch.outx);
  s(latch.pitch);
  s(latch.output);

  for(auto& v : voice) s(v);
}

auto DSP::Voice::serialize(serializer& s) -> void {
  s(index);

  s(volume);
  s(pitch);
  s(source);
  s(adsr0);
  s(adsr1);
  s(gain);
  s(envx);
  s(keyon);
  s(keyoff);
  s(modulate);
  s(noise);
  s(echo);
  s(end);

  s(buffer);
  s(bufferOffset);
  s(gaussianOffset);
  s(brrAddress);
  s(brrOffset);
  s(keyonDelay);
  s(envelopeMode);
  s(envelope);

  s(_envelope);
  s(_keylatch);
  s(_keyon);
  s(_keyoff);
  s(_modulate);
  s(_noise);
  s(_echo);
  s(_end);
  s(_looped);
}
