AtariVox::AtariVox(Node::Port parent) : SaveKey(parent, "AtariVox", "atarivox.eeprom") {
  stream = node->append<Node::Audio::Stream>("AtariVox");
  stream->setChannels(1);
  stream->setFrequency(SpeakJet::Frequency);

  speakjet.reset();
  serialBits = 0;
  serialData = 0;
  serialValid = 1;
  serialCycle = 0;
  Thread::create(SpeakJet::Frequency, std::bind_front(&AtariVox::main, this));
}

AtariVox::~AtariVox() {
  Thread::destroy();
  stream.reset();
}

auto AtariVox::main() -> void {
  stream->frame(speakjet.clock());
  Thread::step(1);
  Thread::synchronize(cpu);
}

auto AtariVox::power(bool reset) -> void {
  SaveKey::power(reset);
  speakjet.reset();
  serialBits = 0;
  serialData = 0;
  serialValid = 1;
  serialCycle = 0;
  Thread::create(SpeakJet::Frequency, std::bind_front(&AtariVox::main, this));
}

auto AtariVox::read() -> n8 {
  n8 data = SaveKey::read();
  data.bit(1) = speakjet.ready();
  return data;
}

auto AtariVox::write(n8 data) -> void {
  SaveKey::write(data);
  receiveSerial(data.bit(0));
}

auto AtariVox::serialize(serializer& s) -> void {
  SaveKey::serialize(s);
  Thread::serialize(s);
  speakjet.serialize(s);
  s(serialBits);
  s(serialData);
  s(serialValid);
  s(serialCycle);
}

auto AtariVox::receiveSerial(bool level) -> void {
  if(!cpu.scalar()) return;
  //The CPU Thread advances three color clocks per 6507 cycle. Stella's serial
  //thresholds are expressed in CPU cycles, so compare in that same domain.
  auto cycle = cpu.clock() / cpu.scalar() / 3;

  if(serialBits && (cycle < serialCycle || cycle > serialCycle + 1000)) {
    serialBits = 0;
    serialData = 0;
    serialValid = 1;
  }

  if(!serialBits && level) {
    serialCycle = cycle;
    return;
  }

  if(serialBits && cycle >= serialCycle && cycle < serialCycle + 62) {
    serialCycle = cycle;
    return;
  }

  if(serialBits == 0) {
    serialValid = !level;
  } else if(serialBits <= 8) {
    serialData.bit(serialBits - 1) = level;
  } else {
    serialValid &= level;
  }

  serialCycle = cycle;
  if(++serialBits < 10) return;
  if(serialValid) speakjet.enqueue(serialData);
  serialBits = 0;
  serialData = 0;
  serialValid = 1;
}
