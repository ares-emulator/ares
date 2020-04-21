//Ricoh RF5C164

auto MCD::PCM::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("PCM");
  stream->setChannels(2);
  stream->setFrequency(12'500'000 / 384.0);

  ram.allocate(64_KiB);
}

auto MCD::PCM::unload() -> void {
  stream = {};

  ram.reset();
}

auto MCD::PCM::clock() -> void {
  int left  = 0;
  int right = 0;

  if(io.enable)
  for(auto& channel : channels) {
    if(!channel.enable) continue;
    int data = ram[channel.address >> 11];
    if(data == 0xff) {  //loop?
      channel.address = channel.loop << 11;
      data = ram[channel.loop];
      if(data == 0xff) continue;  //infinite loop; does not output any sound
    }
    channel.address += channel.step;
    if(data & 0x80) {
      data = +(data & 0x7f);
    } else {
      data = -(data & 0x7f);
    }
    left  += data * channel.envelope * uint4(channel.pan >> 0) >> 5;
    right += data * channel.envelope * uint4(channel.pan >> 4) >> 5;
  }

  //clamp to 10-bit DAC output rate
  left  = sclamp<16>(left ) >> 2 & ~15;
  right = sclamp<16>(right) >> 2 & ~15;
  stream->sample(left / 32768.0, right / 32768.0);
}

auto MCD::PCM::read(uint13 address, uint8 data) -> uint8 {
  if(address >= 0x1000 && address <= 0x1fff) {
    data = ram[io.bank << 12 | (uint12)address];
    return data;
  }

  if(address >= 0x0010 && address <= 0x001f) {
    auto& channel = channels[address >> 1 & 7];
    uint16 offset = channel.address >> 11;
    data = offset.byte(address & 1);
    return data;
  }

  return data = 0xff;
}

auto MCD::PCM::write(uint13 address, uint8 data) -> void {
  if(address >= 0x1000 && address <= 0x1fff) {
    ram[io.bank << 12 | (uint12)address] = data;
    return;
  }

  auto& channel = channels[io.channel];

  switch(address) {

  case 0x0: {  //ENV
    channel.envelope = data;
  } break;

  case 0x1: {  //PAN
    channel.pan = data;
  } break;

  case 0x2: {  //FDL
    channel.step.byte(0) = data;
  } break;

  case 0x3: {  //FDH
    channel.step.byte(1) = data;
  } break;

  case 0x4: {  //LSL
    channel.loop.byte(0) = data;
  } break;

  case 0x5: {  //LSH
    channel.loop.byte(1) = data;
  } break;

  case 0x6: {  //ST
    channel.start = data;
    if(!channel.enable) channel.address = channel.start << 8 << 11;
  } break;

  case 0x7: {  //CTRL
    if(data.bit(6)) {
      io.channel = data.bit(0,2);
    } else {
      io.bank = data.bit(0,3);
    }
    io.enable = data.bit(7);
  } break;

  case 0x8: {  //ONOFF
    for(auto& channel : channels) {
      channel.enable = !(data & 1); data >>= 1;
      if(!channel.enable) channel.address = channel.start << 8 << 11;
    }
  } break;

  }
}

auto MCD::PCM::power(bool reset) -> void {
  ram.fill(0x00);
  io = {};
  for(auto& channel : channels) channel = {};
}
