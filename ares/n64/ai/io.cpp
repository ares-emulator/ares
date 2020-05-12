static const vector<string> registerNames = {
  "AI_DRAM_ADDRESS",
  "AI_LENGTH",
  "AI_CONTROL",
  "AI_STATUS",
  "AI_DACRATE",
  "AI_BITRATE",
};

auto AI::readIO(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //AI_DRAM_ADDRESS (read-only)
  }

  if(address == 1) {
    //AI_LENGTH
    if(io.fifoPending > 0) {
      auto& fifo = this->fifo[io.fifoPlaying];
      bit<0,17>(data) = fifo.length;
    }
  }

  if(address == 2) {
    //AI_CONTROL
    bit<0>(data) = io.dmaEnable;
  }

  if(address == 3) {
    //AI_STATUS
    bit< 0>(data) = io.fifoPending == 2;
    bit<30>(data) = io.fifoPending >= 1 && io.dmaEnable;
    bit<31>(data) = io.fifoPending == 2;
  }

  if(address == 4) {
    //AI_DACRATE
    bit<0,13>(data) = io.dacRate;
  }

  if(address == 5) {
    //AI_BITRATE
    bit<0,3>(data) = io.bitRate;
  }

//print("* ", registerNames(address, "AI_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto AI::writeIO(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //AI_DRAM_ADDRESS
    if(io.fifoPending < 2) {
      auto& fifo = this->fifo[io.fifoWriting];
      fifo.address = bit<0,23>(data) & ~7;
    }
  }

  if(address == 1) {
    //AI_LENGTH
    if(io.fifoPending < 2) {
      auto& fifo = this->fifo[io.fifoWriting];
      fifo.length = bit<0,17>(data) & ~7;
      fifo.offset = 0;
      for(u32 offset = 0; offset < fifo.length; offset += 4) {
        u32 data = rdram.ram.readWord(fifo.address + offset);
        fifo.buffer[fifo.offset++] = data;
      }
      fifo.offset = 0;
      io.fifoWriting ^= 1;
      io.fifoPending++;
    }
  }

  if(address == 2) {
    //AI_CONTROL
    io.dmaEnable = bit<0>(data);
  }

  if(address == 3) {
    //AI_STATUS
    mi.lower(MI::IRQ::AI);
  }

  if(address == 4) {
    //AI_DACRATE
    auto frequency = dac.frequency;
    io.dacRate = bit<0,13>(data);
    dac.frequency = max(1, clockNTSC / (io.dacRate + 1));
    dac.period = 93'750'000 / dac.frequency;
    if(frequency != dac.frequency) stream->setFrequency(dac.frequency);
  }

  if(address == 5) {
    //AI_BITRATE
    io.bitRate = bit<0,3>(data);
    dac.precision = io.bitRate + 1;
  }

//print("* ", registerNames(address, "AI_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
