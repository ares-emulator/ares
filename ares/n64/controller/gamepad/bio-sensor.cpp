auto Gamepad::BioSensor::load() -> void {
  beatsPerMinute = 60;  //default to 60 BPM
  pulseNext = chrono::microsecond();
}

auto Gamepad::BioSensor::unload() -> void {
  pulseNext = 0;
  pulseStart = 0;
  isPulsing = false;
}

auto Gamepad::BioSensor::update() -> void {
  //Calculate pulse interval in microseconds
  static constexpr u64 PULSE_DURATION = 200'000;
  u64 pulseInterval = 60'000'000 / beatsPerMinute;
  u64 now = chrono::microsecond();

  //Check if current pulse should end
  if(isPulsing && (now - pulseStart >= PULSE_DURATION)) {
    isPulsing = false;
  }

  //Check if it's time to start a new pulse
  if(!isPulsing && now >= pulseNext) {
    isPulsing = true;
    pulseStart = now;
    pulseNext = now + pulseInterval;
  }
}

auto Gamepad::BioSensor::read(u16 address) -> u8 {
  //Pulse register: 0x00 = pulsing, 0x03 = resting
  if(address >= 0xC000) return isPulsing ? 0x00 : 0x03;
  //Probe register: 0x81 = accessory ID for Bio Sensor
  if(address >= 0x8000) return 0x81;
  return 0x00;
}
