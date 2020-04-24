auto PCD::ADPCM::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("ADPCM");
  stream->setChannels(1);
  stream->setFrequency(32000);

  memory.allocate(64_KiB);
}

auto PCD::ADPCM::unload() -> void {
  stream = {};

  memory.reset();
}

auto PCD::ADPCM::clock() -> void {
  if(write.pending && !--write.pending) {
    irq.halfReached.line = length < 0x8000;
    if(!io.lengthLatch && length < 0xffff) length++;
    memory.write(write.address++, write.data);
  }

  if(dmaActive && !write.pending) {
    if(auto data = scsi->clockSample()) {
      write.pending = WriteLatency;
      write.data = data();
    }
  }

  if(read.pending && !--read.pending) {
    read.data = memory.read(read.address++);
    irq.halfReached.line = length < 0x8000;
    if(!io.lengthLatch) {
      if(length) {
        length--;
      } else {
        irq.halfReached.lower();
        irq.endReached.raise();
        if(io.noRepeat) io.playing = 0;
      }
    }
  }
}

auto PCD::ADPCM::clockSample() -> void {
  stream->sample((io.playing ? msm5205.sample() : (int12)0) * fader->adpcm() / 2048.0);

  if(++period < divider) return;
  period = 0;

  if(io.playing) {
    if(!sample.nibble++) {
      sample.data = memory.read(read.address++);
      if(!io.lengthLatch && length) length--;
    }
    msm5205.setData(sample.data >> 4);
    msm5205.clock();
    sample.data <<= 4;

    irq.halfReached.line = length < 0x8000;
    if(!io.lengthLatch && !length) {
      irq.halfReached.lower();
      irq.endReached.raise();
      if(io.noRepeat) io.playing = 0;
    }
  }
}

auto PCD::ADPCM::control(uint8 data) -> void {
  io.writeOffset = data.bit(0);
  if(!io.writeLatch && data.bit(1)) {
    write.address = latch - !io.writeOffset;
  }
  io.writeLatch = data.bit(1);

  io.readOffset = data.bit(2);
  if(!io.readLatch && data.bit(3)) {
    read.address = latch - !io.readOffset;
  }
  io.readLatch = data.bit(3);

  io.lengthLatch = data.bit(4);
  if(io.lengthLatch) {
    irq.endReached.lower();
    length = latch;
  }

  if(io.playing && !data.bit(5)) {
    io.playing = 0;
  }
  if(!io.playing && data.bit(5)) {
    irq.halfReached.lower();
    io.playing = 1;
    sample = {};
  }

  io.noRepeat = data.bit(6);
  if(irq.endReached.line && io.noRepeat) {
    io.playing = 0;
  }

  io.reset = data.bit(7);
  if(io.reset) {
    irq.halfReached.lower();
    irq.endReached.lower();
    io = {};
    read = {};
    write = {};
    sample = {};
    latch = 0;
    length = 0;
  }
}

auto PCD::ADPCM::power() -> void {
  msm5205.power();
  msm5205.setReset(0);
  msm5205.setWidth(1);   //4-bit samples
  msm5205.setScaler(2);  //1/48th rate

  irq = {};
  io = {};
  read = {};
  write = {};
  sample = {};
  dmaActive = 0;
  divider = 1;
  period = 0;
  latch = 0;
  length = 0;
}
