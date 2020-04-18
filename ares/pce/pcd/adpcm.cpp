auto PCD::ADPCM::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("ADPCM Stream");
  stream->setChannels(1);
  stream->setFrequency(32000);

  memory.allocate(64_KiB);
}

auto PCD::ADPCM::unload() -> void {
  stream = {};

  memory.reset();
}

auto PCD::ADPCM::clock() -> void {
  if(++counter < 6 * msm5205.scaler()) return;
  counter = 0;

  stream->sample(msm5205.sample() / 2048.0);

  if(++period < divider) return;
  period = 0;

  auto data = memory.read(startAddress);
  msm5205.setData(!nibble ? uint4(data >> 4) : uint4(data >> 0));
  msm5205.clock();

  nibble = !nibble;
  if(!nibble) {
    startAddress++;
    if(startAddress == halfAddress) {
      irq.halfPlay.pending = 1;
    }
    if(startAddress >= endAddress) {
      msm5205.setReset(1);
      stop(1);
    }
  }
}

auto PCD::ADPCM::control(uint8 shadow, uint8 data) -> void {
  if(enable && !data.bit(7)) {
    msm5205.setReset(1);
    stop(0);
    readAddress = 0;
    writeAddress = 0;
    startAddress = 0;
    halfAddress = 0;
    endAddress = 0;
  }

  if(!run && data.bit(6)) {
    msm5205.setReset(0);
    play();
    startAddress = readAddress;
    halfAddress = readAddress + length / 2;
    endAddress = readAddress + length;
  }

  if(!data.bit(6)) {
    msm5205.setReset(1);
    stop(0);
  }

  repeat = data.bit(5);
  run    = data.bit(6);
  enable = data.bit(7);

  if(data.bit(4)) {
    length = latch;
  }

  if(data.bit(3)) {
    readAddress = latch;
    readCounter = 2;
  }

  if(data.bit(1)) {
    writeAddress = latch;
    writeCounter = data.bit(0);
  }
}

auto PCD::ADPCM::play() -> void {
  idle = 0;
  nibble = 0;
  stopped = 0;
  playing = 1;
  irq.fullPlay.pending = 0;
}

auto PCD::ADPCM::stop(bool withIRQ) -> void {
  idle = 1;
  repeat = 0;
  run = 0;
  stopped = 1;
  playing = 0;
  if(withIRQ) irq.fullPlay.pending = 1;
}

auto PCD::ADPCM::power() -> void {
  msm5205.power();
  msm5205.setReset(1);
  msm5205.setWidth(1);   //4-bit
  msm5205.setScaler(2);  //1/48th

  counter = 0;
  irq = {};
  idle = 1;
  nibble = 0;
  enable = 0;
  run = 0;
  stopped = 1;
  playing = 0;
  reading = 0;
  writing = 0;
  repeat = 0;
  divider = 1;
  period = 0;
  latch = 0;
  length = 0;
  readCounter = 0;
  readAddress = 0;
  writeCounter = 0;
  writeAddress = 0;
  startAddress = 0;
  halfAddress = 0;
  endAddress = 0;
}

auto PCD::ADPCM::readRAM() -> uint8 {
  if(!readCounter) return memory.read(readAddress++);
  readCounter--;
  return 0x00;
}

auto PCD::ADPCM::writeRAM(uint8 data) -> void {
  if(!writeCounter) return memory.write(writeAddress++, data);
  writeCounter--;
}
