auto Gamepad::BioSensor::load() -> void {
  beatsPerMinute = 60;  //default to 60 BPM
  nextPulseTime = chrono::microsecond();
}

auto Gamepad::BioSensor::unload() -> void {
  //nothing to do
}

auto Gamepad::BioSensor::update() -> void {
  //Calculate pulse interval in microseconds
  static constexpr u64 PULSE_DURATION = 200'000;
  u64 pulseInterval = 60'000'000 / beatsPerMinute;
  u64 currentTime = chrono::microsecond();

  //Check if current pulse should end
  if(isPulsing && (currentTime - pulseStartTime >= PULSE_DURATION)) {
    isPulsing = false;
  }

  //Check if it's time to start a new pulse
  if(!isPulsing && currentTime >= nextPulseTime) {
    isPulsing = true;
    pulseStartTime = currentTime;
    nextPulseTime = currentTime + pulseInterval;
  }
}

auto Gamepad::BioSensor::read(u16 address) -> u8 {
  // Probe data
  if(address >= 0x8000 && address < 0x8020) {
    return 0x81;
  }

  // Pulse data
  if(address >= 0xC000 && address < 0xC020) {
    return isPulsing ? 0x00 : 0x03;
  }

  //All other addresses return 0x00
  return 0x00;
}

auto Gamepad::BioSensor::write(u16 address, u8 data) -> void {
  //Bio Sensor ignores all writes
  return;
}

auto Gamepad::BioSensor::setBeatsPerMinute(u8 bpm) -> void {
  if(bpm < 30) bpm = 30;    //minimum 30 BPM
  if(bpm > 240) bpm = 240;  //maximum 240 BPM
  beatsPerMinute = bpm;
}
