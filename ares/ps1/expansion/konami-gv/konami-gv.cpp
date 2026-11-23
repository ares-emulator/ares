KonamiGV::KonamiGV(Node::Object parent) {
  port = parent->append<Node::Object>("Arcade");
  controls = port->append<Node::Object>("Controls");
  for(u32 player : range(4)) {
    string prefix = {"Player ", player + 1, " "};
    up[player]    = controls->append<Node::Input::Button>(string{prefix, "Up"});
    down[player]  = controls->append<Node::Input::Button>(string{prefix, "Down"});
    left[player]  = controls->append<Node::Input::Button>(string{prefix, "Left"});
    right[player] = controls->append<Node::Input::Button>(string{prefix, "Right"});
    for(u32 index : range(3)) button[player][index] = controls->append<Node::Input::Button>(string{prefix, "Button ", index + 1});
    start[player] = controls->append<Node::Input::Button>(string{prefix, "Start"});
  }
  coin[0] = controls->append<Node::Input::Button>("Player 1 Coin");
  coin[1] = controls->append<Node::Input::Button>("Player 2 Coin");
  service = controls->append<Node::Input::Button>("Service");
  test = controls->append<Node::Input::Button>("Test");

  auto gameName = system.pak->attribute("name");
  if(gameName == "btchamp") mode = Mode::Trackball2;
  if(gameName == "simpbowl") mode = Mode::Trackball1;
  if(gameName == "kdeadeye") mode = Mode::Lightgun;
  if(auto fp = system.pak->read("save.flash")) {
    if(fp->size() == 8_MiB) {
      hasFujitsuFlash = true;
      for(auto& chip : fujitsu) chip.data.assign(2_MiB, 0xff);
      fp->seek(0);
      for(auto& chip : fujitsu) fp->read(chip.data.data(), chip.data.size());
    }
  }
  bool sharpSaved = false;
  if(gameName == "btchamp" || gameName == "kdeadeye") {
    hasSharpFlash = true;
    sharp.data.assign(512_KiB, 0xff);
    if(auto fp = system.pak->read("flash.rom")) {
      fp->seek(0);
      fp->read(sharp.data.data(), min<u64>(fp->size(), sharp.data.size()));
    }
    if(auto fp = system.pak->read("save.flash")) {
      if(fp->size() == sharp.data.size()) {
        std::vector<u8> saved(sharp.data.size());
        fp->seek(0);
        fp->read(saved.data(), saved.size());
        for(u8 byte : saved) {
          if(byte != 0xff) { sharp.data = saved; sharpSaved = true; break; }
        }
      }
    }
  }
  cdStream = port->append<Node::Audio::Stream>("CD Audio");
  cdStream->setChannels(2);
  cdStream->setFrequency(44100.0);
  if(mode == Mode::Lightgun) {
    for(u32 player : range(2)) {
      crosshair[player] = port->append<Node::Video::Sprite>(string{"Player ", player + 1, " Crosshair"});
      nall::image image = player == 0 ? nall::image{Resource::Sprite::SuperFamicom::CrosshairGreen} : nall::image{Resource::Sprite::SuperFamicom::CrosshairRed};
      u32 width = image.width() * 8;
      u32 height = image.height() * 2;
      image.scale(width, height, false);
      crosshair[player]->setImage(image);
      gpu.screen->attach(crosshair[player]);
    }
  }
  if(mode != Mode::None) {
    for(u32 player : range(2)) {
      axisX[player] = controls->append<Node::Input::Axis>(string{"Player ", player + 1, " X-Axis"});
      axisY[player] = controls->append<Node::Input::Axis>(string{"Player ", player + 1, " Y-Axis"});
      if(mode != Mode::None) {
        motionX[player] = controls->append<Node::Input::Axis>(string{"Player ", player + 1, " X-Axis (Mouse)"});
        motionY[player] = controls->append<Node::Input::Axis>(string{"Player ", player + 1, " Y-Axis (Mouse)"});
      }
    }
  }

  eeprom.allocate(128, 16, 0, 0xff);
  if(auto fp = system.pak->read("save.eeprom")) {
    fp->seek(0);
    fp->read(eeprom.data, min<u64>(fp->size(), 128));
  }
  if(hasSharpFlash && !sharpSaved) {
    for(u32 index : range(128)) eeprom.data[index] = 0xff;
    if(auto fp = system.pak->read("eeprom.rom")) {
      fp->seek(0);
      fp->read(eeprom.data, min<u64>(fp->size(), 128));
    }
  }
  if(hasFujitsuFlash) {
    bool blank = true;
    for(auto& chip : fujitsu) {
      for(u8 byte : chip.data) {
        if(byte != 0xff) { blank = false; break; }
      }
    }
    if(blank) {
      eeprom.data[2] = 0x00;
      eeprom.data[3] = 0x01;
    }
  }

  cdrom.session = []() -> CD::Session* { return disc.fd ? &disc.session : nullptr; };
  cdrom.readSector = [](s32 lba, u8* out) -> bool {
    if(!disc.fd) return false;
    disc.fd->seek(2448ull * (CD::LeadInSectors + CD::LBAtoABA(lba)));
    disc.fd->read({out, 2352});
    return true;
  };
  scsi.devices[4] = &cdrom;
  scsi.irq = [](bool line) { interrupt.drive(Interrupt::PIO, line); };
}

auto KonamiGV::flashRead(u32 offset, u32& value) -> bool {
  if(hasSharpFlash && offset >= 0x38'0000 && offset <= 0x3f'ffff) {
    value = sharp.read((offset & 0x7ffff) >> 1);
    return true;
  }
  if(hasFujitsuFlash && offset >= 0x68'0080 && offset <= 0x68'008f) {
    value = 0;
    u32 reg = (offset - 0x68'0080) >> 1;
    if(reg == 0) {
      u32 pair = flashAddress >= 0x20'0000 ? 2 : 0;
      value = fujitsu[pair].read(flashAddress) | fujitsu[pair + 1].read(flashAddress) << 8;
      flashAddress++;
    } else if(reg == 4) {
      flashAddress |= 1;
    }
    return true;
  }
  return false;
}

auto KonamiGV::flashWrite(u32 offset, u32 value) -> bool {
  if(hasSharpFlash && offset >= 0x38'0000 && offset <= 0x3f'ffff) {
    sharp.write((offset & 0x7ffff) >> 1, value);
    return true;
  }
  if(hasFujitsuFlash && offset >= 0x68'0080 && offset <= 0x68'008f) {
    u32 reg = (offset - 0x68'0080) >> 1;
    value &= 0xffff;
    if(reg == 0) {
      u32 pair = flashAddress >= 0x20'0000 ? 2 : 0;
      fujitsu[pair].write(flashAddress, value);
      fujitsu[pair + 1].write(flashAddress, value >> 8);
    } else if(reg == 1) {
      flashAddress = value << 1;
    } else if(reg == 2) {
      flashAddress &= ~0xff00u;
      flashAddress |= (value & 0xff) << 8;
      flashAddress |= (value >> 8) << 16;
    } else if(reg == 3) {
      flashAddress &= 0xffff;
      flashAddress |= value << 15;
    }
    return true;
  }
  return false;
}

KonamiGV::~KonamiGV() {
  for(auto& sprite : crosshair) {
    if(gpu.screen && sprite) gpu.screen->detach(sprite);
  }
}

auto KonamiGV::updateCrosshair(u32 player) -> void {
  auto& screen = gpu.screen;
  bool pal = gpu.io.videoMode;
  s64 left = max<s64>(gpu.io.displayRangeX1, 488) - 488;
  s64 right = min<s64>(gpu.io.displayRangeX2, pal ? 3300 : 3288) - 488;
  s64 activeTop = pal ? 20 : 16;
  s64 top = (max<s64>(gpu.io.displayRangeY1, activeTop) - activeTop) * 2;
  s64 bottom = (min<s64>(gpu.io.displayRangeY2, pal ? 308 : 256) - activeTop) * 2;
  if(right <= left || bottom <= top) {
    crosshair[player]->setVisible(false);
    return;
  }
  s64 x = left + (s64)gunX[player] * (right - left) / 65536;
  s64 y = top + (s64)gunY[player] * (bottom - top) / 65536;
  x -= crosshair[player]->width() / 2;
  y -= crosshair[player]->height() / 2;
  bool onscreen = x >= 0 && y >= 0 && x + crosshair[player]->width() <= screen->canvasWidth() && y + crosshair[player]->height() <= screen->canvasHeight();
  crosshair[player]->setVisible(onscreen && gunActive[player]);
  if(onscreen) crosshair[player]->setPosition(x, y);
}

auto KonamiGV::save() -> void {
  if(hasFujitsuFlash) {
    if(auto fp = system.pak->write("save.flash")) {
      fp->seek(0);
      for(auto& chip : fujitsu) fp->write(chip.data.data(), chip.data.size());
    }
  }
  if(hasSharpFlash) {
    if(auto fp = system.pak->write("save.flash")) {
      fp->seek(0);
      fp->write(sharp.data.data(), sharp.data.size());
    }
  }
  if(auto fp = system.pak->write("save.eeprom")) {
    fp->seek(0);
    fp->write(eeprom.data, 128);
  }
}

auto KonamiGV::power(bool reset) -> void {
  eeprom.power();
  eeprom.select = 0;
  eeprom.clock = 0;
  scsi.power();
  cdrom.power();
  for(auto& chip : fujitsu) chip.power();
  sharp.power();
  flashAddress = 0;
  audioAccumulator = 0;
  audioSample = 0;
  audioSectorLBA = -1;
  clockAccumulator = 0;
  frameAccumulator = 0;
  for(auto& chip : trackball) chip.power();
  for(u32 player : range(2)) {
    gunX[player] = gunY[player] = 32768;
    gunActive[player] = player == 0;
    if(crosshair[player]) updateCrosshair(player);
  }
  p1 = p2 = p34 = 0xffff'ffff;
}

auto KonamiGV::step(u32 clocks) -> void {
  if(mode == Mode::Lightgun) {
    frameAccumulator += clocks;
    u32 frame = system.frequency() / 60;
    while(frameAccumulator >= frame) {
      frameAccumulator -= frame;
      for(u32 player : range(2)) {
        if(!axisX[player]) continue;
        platform->input(axisX[player]);
        platform->input(axisY[player]);
        platform->input(motionX[player]);
        platform->input(motionY[player]);
        s32 deltaX = ((s32)axisX[player]->value() >> 11) + (s32)motionX[player]->value();
        s32 deltaY = ((s32)axisY[player]->value() >> 11) + (s32)motionY[player]->value();
        if(deltaX || deltaY) gunActive[player] = true;
        gunX[player] = max<s32>(-3000, min<s32>(65535 + 3000, gunX[player] + deltaX * 100));
        gunY[player] = max<s32>(-2000, min<s32>(65535 + 2000, gunY[player] + deltaY * 100));
        updateCrosshair(player);
      }
    }
  }
  if(mode == Mode::Trackball1 || mode == Mode::Trackball2) {
    frameAccumulator += clocks;
    u32 frame = system.frequency() / 60;
    while(frameAccumulator >= frame) {
      frameAccumulator -= frame;
      for(u32 player : range(2)) {
        if(!axisX[player]) continue;
        platform->input(axisX[player]);
        platform->input(axisY[player]);
        platform->input(motionX[player]);
        platform->input(motionY[player]);
        motionRemainderX[player] += (s32)motionX[player]->value();
        motionRemainderY[player] += (s32)motionY[player]->value();
        s32 mouseX = motionRemainderX[player] / 4;
        s32 mouseY = motionRemainderY[player] / 4;
        motionRemainderX[player] -= mouseX * 4;
        motionRemainderY[player] -= mouseY * 4;
        s32 deltaX = ((s32)axisX[player]->value() >> 11) + mouseX;
        s32 deltaY = ((s32)axisY[player]->value() >> 11) + mouseY;
        trackball[player].addX(mode == Mode::Trackball1 ? -deltaX : deltaX);
        trackball[player].addY(deltaY);
      }
    }
  }
  audioAccumulator += clocks;
  u32 audioFrame = system.frequency() / 44100;
  while(audioAccumulator >= audioFrame) {
    audioAccumulator -= audioFrame;
    if(cdrom.audioStatus != 0x11) {
      cdStream->frame(0.0, 0.0);
      audioSample = 0;
      audioSectorLBA = -1;
      continue;
    }
    if(audioSectorLBA != cdrom.audioPosition) {
      audioSectorLBA = cdrom.audioPosition;
      if(!cdrom.readSector || !cdrom.readSector(audioSectorLBA, audioSector)) memory::fill<u8>(audioSector, sizeof(audioSector));
    }
    u32 offset = audioSample * 4;
    s16 left = audioSector[offset + 0] | audioSector[offset + 1] << 8;
    s16 right = audioSector[offset + 2] | audioSector[offset + 3] << 8;
    cdStream->frame(left / 32768.0 * 0.3, right / 32768.0 * 0.3);
    if(++audioSample >= 588) {
      audioSample = 0;
      cdrom.audioTick();
    }
  }
  clockAccumulator += clocks;
  u32 ticks = system.frequency() / 1000;
  while(clockAccumulator >= ticks) {
    clockAccumulator -= ticks;
    eeprom.M93LCx6::clock();
  }
}

auto KonamiGV::serialize(serializer& s) -> void {
  for(auto& chip : fujitsu) chip.serialize(s);
  sharp.serialize(s);
  s(flashAddress);
  for(auto& chip : trackball) chip.serialize(s);
  s(frameAccumulator);
  s(eeprom);
  s(scsi);
  s(cdrom);
  s(clockAccumulator);
}

auto KonamiGV::EEPROM::readPin() -> n1 {
  if(!select) return 1;
  if(busy) return 0;
  if(output.count) return output.edge();
  return 1;
}

auto KonamiGV::EEPROM::writePins(n8 value) -> void {
  bool cs = value.bit(1);
  bool di = value.bit(0);
  bool clk = value.bit(2);
  if(select && !cs) { M93LCx6::power(); select = 0; clock = clk; return; }
  if(!(select = cs)) { clock = clk; return; }
  if(!clock.raise(clk)) return;
  if(output.count) {
    if(input.start() && *input.start() == 1 && input.opcode() && *input.opcode() == 0b10) {
      output.read();
      if(output.count == 0) {
        input.increment();
        read();
        output.read();
      }
    }
    return;
  }
  output.flush();
  input.write(di);
  edge();
}

auto KonamiGV::EEPROM::serialize(serializer& s) -> void {
  M93LCx6::serialize(s);
  s(select);
  s(clock);
}

auto KonamiGV::pollInputs() -> void {
  for(u32 player : range(4)) {
    platform->input(up[player]);
    platform->input(down[player]);
    platform->input(left[player]);
    platform->input(right[player]);
    for(u32 index : range(3)) platform->input(button[player][index]);
    platform->input(start[player]);
  }
  for(auto& input : coin) platform->input(input);
  platform->input(service);
  platform->input(test);

  auto joystick = [&](u32 player) -> u32 {
    u32 bits = 0;
    bits |= up[player]->value()    << 0;
    bits |= down[player]->value()  << 1;
    bits |= left[player]->value()  << 2;
    bits |= right[player]->value() << 3;
    for(u32 index : range(3)) bits |= button[player][index]->value() << 4 + index;
    return bits;
  };

  u32 pressed1 = joystick(0) | start[0]->value() << 9 | coin[0]->value() << 10 | service->value() << 11 | test->value() << 12;
  u32 pressed2 = joystick(1) | start[1]->value() << 9 | coin[1]->value() << 10;
  u32 pressed34 = joystick(2) | start[2]->value() << 7 | joystick(3) << 8 | start[3]->value() << 15;

  p1 = ~pressed1 & 0xffff'dfff;
  p2 = ~pressed2;
  p34 = ~pressed34;
}

auto KonamiGV::lightgunValue(u32 player, bool vertical) -> u32 {
  if(!axisX[player]) return 0;
  s64 raw = vertical ? gunY[player] : gunX[player];
  if(vertical) {
    if(raw < 0) return 0xff;
    return raw * 0xef / 65535;
  }
  if(raw < 0) return 0;
  return 0x4c + raw * (0x1bb - 0x4c) / 65535;
}

auto KonamiGV::readRegister(u32 offset) -> u32 {
  if(offset < 0x20) return scsi.read(offset >> 1);
  if(mode == Mode::Lightgun && offset >= 0x68'0080 && offset <= 0x68'00c3) {
    pollInputs();
    if(offset == 0x68'0080) return lightgunValue(0, false);
    if(offset == 0x68'0090) return lightgunValue(0, true);
    if(offset == 0x68'00a0) return lightgunValue(1, false);
    if(offset == 0x68'00b0) return lightgunValue(1, true);
    if(offset == 0x68'00c0) return ~(u32)(button[0][0]->value() | button[1][0]->value() << 1) | 0xffff'fffc;
  }
  if(mode == Mode::Trackball2 && (offset == 0x68'0080 || offset == 0x68'0084)) {
    u32 base = offset == 0x68'0084 ? 2 : 0;
    return trackball[1].readSelected(base) << 0 | trackball[0].readSelected(base) << 8
         | trackball[1].readSelected(base + 1) << 16 | trackball[0].readSelected(base + 1) << 24;
  }
  if(mode == Mode::Trackball1 && (offset == 0x68'00c0 || offset == 0x68'00c4)) {
    u32 base = offset == 0x68'00c4 ? 2 : 0;
    return trackball[0].readSelected(base) << 8 | trackball[0].readSelected(base + 1) << 24;
  }
  if(mode == Mode::Trackball1 && offset == 0x68'00c8) {
    trackball[0].resetPulse(1, 1);
    return 0;
  }
  if(offset >= 0x10'0000 && offset <= 0x10'000b) {
    pollInputs();
    if(offset < 0x10'0004) return p1 & ~(1u << 13) | (u32)eeprom.readPin() << 13;
    if(offset < 0x10'0008) return p2;
    return p34;
  }
  if(offset >= 0x68'0000 && offset < 0x68'0100) return 0;
  return 0xffff'ffff;
}

auto KonamiGV::writeRegister(u32 offset, u32 data) -> void {
  if(offset < 0x20) return scsi.write(offset >> 1, data);
  if(offset == 0x18'0000) {
    return eeprom.writePins(data);
  }
  if(mode == Mode::Trackball2 && offset == 0x68'0088) {
    for(auto& chip : trackball) {
      chip.setChipSelect(data >> 1 & 1);
      chip.setResetX(!(data & 1));
      chip.setResetY(!(data & 1));
    }
  }
}

auto KonamiGV::readHalf(u32 address) -> u32 {
  u32 offset = address & 0x7f'fffe;
  if(u32 value; flashRead(offset, value)) return value;
  if(offset < 0x20) return readRegister(offset) & 0xff;
  u32 value = readRegister(offset & ~3);
  return offset & 2 ? value >> 16 : value & 0xffff;
}

auto KonamiGV::readByte(u32 address) -> u32 {
  return readHalf(address & ~1) >> 8 * (address & 1) & 0xff;
}

auto KonamiGV::readWord(u32 address) -> u32 {
  u32 offset = address & 0x7f'fffc;
  if(hasSharpFlash && offset >= 0x38'0000 && offset <= 0x3f'ffff) return readHalf(address) | readHalf(address + 2) << 16;
  if(hasFujitsuFlash && offset >= 0x68'0080 && offset <= 0x68'008f) return readHalf(address) | readHalf(address + 2) << 16;
  if(offset < 0x20) return readHalf(address) | readHalf(address + 2) << 16;
  return readRegister(offset);
}

auto KonamiGV::writeHalf(u32 address, u32 data) -> void {
  u32 offset = address & 0x7f'fffe;
  if(flashWrite(offset, data)) return;
  if(offset < 0x20) return writeRegister(offset, data & 0xff);
  writeRegister(offset & ~3, data & 0xffff);
}

auto KonamiGV::writeByte(u32 address, u32 data) -> void {
  u32 offset = address & 0x7f'ffff;
  if(offset < 0x20) { if(!(offset & 1)) writeRegister(offset, data & 0xff); return; }
  writeRegister(offset & ~3, data & 0xff);
}

auto KonamiGV::writeWord(u32 address, u32 data) -> void {
  u32 offset = address & 0x7f'fffc;
  if((hasSharpFlash && offset >= 0x38'0000 && offset <= 0x3f'ffff) || (hasFujitsuFlash && offset >= 0x68'0080 && offset <= 0x68'008f)) { writeHalf(address, data); writeHalf(address + 2, data >> 16); return; }
  if(offset < 0x20) { writeHalf(address, data); writeHalf(address + 2, data >> 16); return; }
  writeRegister(offset, data);
}

auto KonamiGV::canReadDMA() -> bool { return scsi.canDMARead(); }
auto KonamiGV::canWriteDMA() -> bool { return scsi.canDMAWrite(); }

auto KonamiGV::readDMA() -> u32 {
  u32 word = 0;
  for(u32 byte : range(4)) word |= scsi.dmaRead() << 8 * byte;
  return word;
}

auto KonamiGV::writeDMA(u32 data) -> void {
  for(u32 byte : range(4)) scsi.dmaWrite(data >> 8 * byte);
}
