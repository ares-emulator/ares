namespace {

constexpr u16 AllophoneDuration[] = {
   70,  70,  70,  70,  70,  70,  70,  70,  70,  70,  70,  70,
   70,  70,  70,  70,  70,  70,  70,  70,  70, 200, 200, 190,
  200, 185, 165, 200, 225, 185, 170, 140, 180, 170, 170, 200,
  131,  70,  70,  70,  70,  70,  45,  45,  10,  10,  45,  45,
   10,  10,  55,  55,  55,  55,  70,  70,  70,  70,  70,  40,
   40,  50,  40,  50,  70, 170,  55,  55,  55,  45,  99,  99,
};

constexpr u16 EffectDuration[] = {
   80,  80,  80,  80,  80,  80,  80,  80,  80,  80,
  300, 101, 102, 540, 530, 500, 135, 600, 300, 250,
  200, 270, 280, 260, 300, 100, 104, 100, 270, 262,
  160, 300, 182, 120, 175, 350, 160, 260,  95,  75,
   95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,  95,
  125, 250, 530,
};

constexpr u16 PauseDuration[] = {0, 100, 200, 700, 30, 60, 90};

static_assert(std::size(AllophoneDuration) == 72);
static_assert(std::size(EffectDuration) == 55);

}

auto SpeakJet::reset() -> void {
  std::fill(std::begin(fifo), std::end(fifo), 0);
  fifoRead = 0;
  fifoWrite = 0;
  fifoCount = 0;
  parameterPending = 0;
  pendingCommand = 0;
  waiting = 0;
  volume = 96;
  speed = 114;
  pitch = 88;
  bend = 5;
  portControl = 7;
  portOutput = 0;
  repeatRequest = 0;
  repeatCode = 0;
  repeatRemaining = 0;
  nextDurationScale = 1.0;
  nextPitchScale = 1.0;
  soundCode = 0;
  soundClass = SoundClass::None;
  soundSamples = 0;
  soundRemaining = 0;
  soundPitchScale = 1.0;
  std::fill(std::begin(phase), std::end(phase), 0.0);
  for(auto& stage : filter) std::fill(std::begin(stage), std::end(stage), 0.0);
  noiseLfsr = 0x4a3d;
}

auto SpeakJet::enqueue(u8 data) -> bool {
  if(fifoCount == 64) return false;
  fifo[fifoWrite++] = data;
  fifoCount++;
  return true;
}

auto SpeakJet::clock() -> f64 {
  if(!soundRemaining && !waiting) processCommands();
  if(!soundRemaining) return 0.0;

  auto sample = synthesize();
  if(!--soundRemaining) finishSound();
  return sample;
}

auto SpeakJet::serialize(serializer& s) -> void {
  for(auto& byte : fifo) s(byte);
  s(fifoRead);
  s(fifoWrite);
  s(fifoCount);
  s(parameterPending);
  s(pendingCommand);
  s(waiting);
  s(volume);
  s(speed);
  s(pitch);
  s(bend);
  s(portControl);
  s(portOutput);
  s(repeatRequest);
  s(repeatCode);
  s(repeatRemaining);
  s(nextDurationScale);
  s(nextPitchScale);
  s(soundCode);
  s(soundClass);
  s(soundSamples);
  s(soundRemaining);
  s(soundPitchScale);
  for(auto& value : phase) s(value);
  for(auto& stage : filter) for(auto& value : stage) s(value);
  s(noiseLfsr);
}

auto SpeakJet::dequeue() -> u8 {
  if(!fifoCount) return 0;
  auto data = fifo[fifoRead++];
  fifoCount--;
  return data;
}

auto SpeakJet::processCommands() -> void {
  for(u32 commands : range(64)) {
    if(parameterPending) {
      if(!fifoCount) return;
      parameter(pendingCommand, dequeue());
      parameterPending = 0;
      if(soundRemaining || waiting) return;
      continue;
    }

    if(repeatRemaining) {
      execute(repeatCode, true);
      if(soundRemaining || waiting) return;
      continue;
    }

    if(!fifoCount) return;
    execute(dequeue());
    if(soundRemaining || waiting) return;
  }
}

auto SpeakJet::execute(u8 code, bool repeated) -> void {
  if(repeated) repeatRemaining--;

  if(code <= 6) {
    startSound(code);
    return;
  }

  switch(code) {
  case 7: nextDurationScale = 0.5; return;
  case 8: nextDurationScale = 1.5; return;
  case 14: nextPitchScale = 1.08; return;
  case 15: nextPitchScale = 0.92; return;
  case 16: waiting = 1; return;
  case 20: case 21: case 22: case 23: case 24: case 25: case 26: case 28: case 29: case 30:
    pendingCommand = code;
    parameterPending = 1;
    return;
  case 31:
    volume = 96;
    speed = 114;
    pitch = 88;
    bend = 5;
    nextDurationScale = 1.0;
    nextPitchScale = 1.0;
    return;
  case 255:
    repeatRequest = 0;
    repeatRemaining = 0;
    nextDurationScale = 1.0;
    nextPitchScale = 1.0;
    return;
  }

  if(code < 128) return;
  if(!repeated && repeatRequest) {
    repeatCode = code;
    repeatRemaining = repeatRequest;
    repeatRequest = 0;
  }
  startSound(code);
}

auto SpeakJet::parameter(u8 command, u8 value) -> void {
  switch(command) {
  case 20: volume = min(value, (u8)127); break;
  case 21: speed = min(value, (u8)127); break;
  case 22: pitch = value; break;
  case 23: bend = min(value, (u8)15); break;
  case 24: portControl = value & 7; break;
  case 25: portOutput = value & 7; break;
  case 26: repeatRequest = value; break;
  case 28: break;  //Phrase EEPROM execution is outside the behavioral model.
  case 29: break;  //Phrase EEPROM execution is outside the behavioral model.
  case 30:
    soundCode = command;
    soundClass = SoundClass::Pause;
    auto samples = value * Frequency / 100.0;
    soundSamples = soundRemaining = samples > 0.0 ? max(1u, (u32)(samples + 0.5)) : 0;
    break;
  }
}

auto SpeakJet::startSound(u8 code) -> void {
  soundCode = code;
  soundClass = classify(code);
  auto milliseconds = duration(code);
  auto scaled = milliseconds * Frequency / 1000.0;
  scaled *= 114.0 / max(1.0, (f64)speed);
  scaled *= nextDurationScale;
  soundSamples = soundRemaining = scaled > 0.0 ? max(1u, (u32)(scaled + 0.5)) : 0;
  soundPitchScale = nextPitchScale;
  nextDurationScale = 1.0;
  nextPitchScale = 1.0;
  std::fill(std::begin(phase), std::end(phase), 0.0);
  for(auto& stage : filter) std::fill(std::begin(stage), std::end(stage), 0.0);
}

auto SpeakJet::finishSound() -> void {
  soundClass = SoundClass::None;
  soundSamples = 0;
}

auto SpeakJet::synthesize() -> f64 {
  if(soundClass == SoundClass::Pause) return 0.0;
  auto progress = soundSamples ? 1.0 - (f64)soundRemaining / soundSamples : 1.0;
  auto attack = min(1.0, progress * 12.0);
  auto release = min(1.0, (1.0 - progress) * 10.0);
  auto envelope = attack * release;
  auto sample = soundCode < 200 ? synthesizeSpeech(progress) : synthesizeEffect(progress);
  sample *= envelope * ((f64)volume / 127.0);
  return max(-1.0, min(1.0, sample));
}

auto SpeakJet::synthesizeSpeech(f64 progress) -> f64 {
  //The public MSA tables specify identities and durations, not oscillator trajectories.
  //Use a deterministic phonetic-class model rather than importing a third-party voice table.
  auto identity = soundCode - 128;
  auto voiced = soundCode <= 181;
  auto fundamental = (f64)pitch * soundPitchScale;
  auto glottal = fundamental ? oscillator(0, fundamental) * 2.0 - 1.0 : 0.0;
  auto source = voiced ? glottal : 0.0;
  auto breath = noise();

  if(soundClass == SoundClass::Fricative) source = voiced ? 0.4 * glottal + 0.75 * breath : breath;
  if(soundClass == SoundClass::Affricate) source = 0.35 * glottal + 0.85 * breath;
  if(soundClass == SoundClass::Stop) {
    if(progress < 0.45) source = 0.0;
    else source = (voiced ? 0.25 * glottal : 0.0) + breath * (1.0 - progress);
  }
  if(soundClass == SoundClass::Nasal) source = 0.75 * glottal + 0.1 * breath;

  auto bendScale = 0.75 + (f64)bend * 0.05;
  auto formant1 = (330.0 + (identity * 73) % 520) * bendScale;
  auto formant2 = (850.0 + (identity * 97) % 1300) * bendScale;
  auto formant3 = (2100.0 + (identity * 53) % 1050) * bendScale;
  if(soundClass == SoundClass::Nasal) {
    formant1 *= 0.72;
    formant2 *= 0.82;
  }
  if(soundClass == SoundClass::Resonate) formant2 *= 0.88;

  auto low = resonator(0, source, formant1, 110.0);
  auto middle = resonator(1, source, formant2, 170.0);
  auto high = resonator(2, source, formant3, 260.0);
  return 0.95 * low + 0.65 * middle + 0.4 * high;
}

auto SpeakJet::synthesizeEffect(f64 progress) -> f64 {
  auto index = soundCode % 10;

  switch(soundClass) {
  case SoundClass::Robot: {
    auto carrier = oscillator(0, 90.0 + index * 31.0) < 0.5 ? -1.0 : 1.0;
    auto modulator = oscillator(1, 8.0 + index * 2.0) * 2.0 - 1.0;
    return carrier * (0.45 + 0.35 * modulator);
  }
  case SoundClass::Alarm: {
    auto sweep = 250.0 + index * 45.0 + 900.0 * progress;
    return 0.65 * sin(2.0 * Math::Pi * oscillator(0, sweep))
         + 0.25 * sin(2.0 * Math::Pi * oscillator(1, sweep * 1.51));
  }
  case SoundClass::Beep:
    return 0.8 * sin(2.0 * Math::Pi * oscillator(0, 420.0 + index * 145.0));
  case SoundClass::Biological: {
    auto chirp = 130.0 + index * 37.0 + sin(progress * Math::Pi * 6.0) * (90.0 + index * 9.0);
    return 0.6 * sin(2.0 * Math::Pi * oscillator(0, chirp)) + 0.2 * noise();
  }
  case SoundClass::DTMF: {
    static constexpr u16 low[] = {941, 697, 697, 697, 770, 770, 770, 852, 852, 852, 941, 941};
    static constexpr u16 high[] = {1336, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1336, 1477, 1209, 1477};
    auto digit = soundCode - 240;
    return 0.45 * sin(2.0 * Math::Pi * oscillator(0, low[digit]))
         + 0.45 * sin(2.0 * Math::Pi * oscillator(1, high[digit]));
  }
  case SoundClass::Sonar: {
    auto frequency = 1500.0 - 900.0 * progress;
    return sin(2.0 * Math::Pi * oscillator(0, frequency)) * (1.0 - progress);
  }
  case SoundClass::Pistol:
    return noise() * (1.0 - progress) * (1.0 - progress);
  case SoundClass::Wow: {
    auto frequency = 180.0 + 780.0 * sin(progress * Math::Pi);
    return 0.7 * sin(2.0 * Math::Pi * oscillator(0, frequency));
  }
  default: return 0.0;
  }
}

auto SpeakJet::resonator(u32 index, f64 input, f64 frequency, f64 bandwidth) -> f64 {
  frequency = min(frequency, 3999.0);
  auto radius = exp(-Math::Pi * bandwidth / Frequency);
  auto output = (1.0 - radius) * input
              + 2.0 * radius * cos(2.0 * Math::Pi * frequency / Frequency) * filter[index][0]
              - radius * radius * filter[index][1];
  filter[index][1] = filter[index][0];
  filter[index][0] = output;
  return output;
}

auto SpeakJet::oscillator(u32 index, f64 frequency) -> f64 {
  phase[index] += min(frequency, 3999.0) / Frequency;
  if(phase[index] >= 1.0) phase[index] -= 1.0;
  return phase[index];
}

auto SpeakJet::noise() -> f64 {
  auto feedback = (noiseLfsr ^ (noiseLfsr >> 2) ^ (noiseLfsr >> 3) ^ (noiseLfsr >> 5)) & 1;
  noiseLfsr = (noiseLfsr >> 1) | feedback << 15;
  return (noiseLfsr & 1) ? 1.0 : -1.0;
}

auto SpeakJet::duration(u8 code) -> u32 {
  if(code <= 6) return PauseDuration[code];
  if(code >= 128 && code <= 199) return AllophoneDuration[code - 128];
  if(code >= 200 && code <= 254) return EffectDuration[code - 200];
  return 0;
}

auto SpeakJet::classify(u8 code) -> SoundClass {
  if(code <= 6) return SoundClass::Pause;
  if(code >= 128 && (code <= 139 || code >= 149 && code <= 164)) return SoundClass::Vowel;
  if(code >= 140 && code <= 144) return SoundClass::Nasal;
  if(code >= 145 && code <= 148) return SoundClass::Resonate;
  if(code == 165 || code == 182) return SoundClass::Affricate;
  if((code >= 166 && code <= 169) || (code >= 183 && code <= 190)) return SoundClass::Fricative;
  if((code >= 170 && code <= 181) || (code >= 191 && code <= 199)) return SoundClass::Stop;
  if(code >= 200 && code <= 209) return SoundClass::Robot;
  if(code >= 210 && code <= 219) return SoundClass::Alarm;
  if(code >= 220 && code <= 229) return SoundClass::Beep;
  if(code >= 230 && code <= 239) return SoundClass::Biological;
  if(code >= 240 && code <= 251) return SoundClass::DTMF;
  if(code == 252) return SoundClass::Sonar;
  if(code == 253) return SoundClass::Pistol;
  if(code == 254) return SoundClass::Wow;
  return SoundClass::None;
}
