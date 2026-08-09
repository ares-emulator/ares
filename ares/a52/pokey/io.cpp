auto POKEY::read(n8 address) -> n8 {
  return peek(address);
}

auto POKEY::peek(n8 address) const -> n8 {
  address &= 0x0f;
  if(address <= 0x07) return pots.read(address);
  if(address == 0x08) return pots.all();
  if(address == 0x09) return keyboard.code();
  if(address == 0x0a) {
    if(!(control & 3)) return 0xff;
    return clock.random(audio.control.bit(7));
  }
  if(address == 0x0d) return serial.input();
  if(address == 0x0e) return irq.status();
  if(address == 0x0f) return status.read();
  return 0xff;
}

auto POKEY::write(n8 address, n8 data) -> void {
  address &= 0x0f;
  switch(address) {
  case 0x00:
  case 0x02:
  case 0x04:
  case 0x06:
    return audio.writeFrequency(address >> 1, data);
  case 0x01:
  case 0x03:
  case 0x05:
  case 0x07:
    return audio.writeControl(address >> 1, data);
  case 0x08:
    return audio.writeAUDCTL(data);
  case 0x09:
    return audio.startTimers();
  case 0x0a:
    return status.resetErrors();
  case 0x0b:
    return startPots();
  case 0x0c:  //unused write register
    return;
  case 0x0d:
    serial.write(data);
    setIRQ();
    return;
  case 0x0e:
    irq.writeEnable(data);
    setIRQ();
    return;
  case 0x0f:
    return writeSKCTL(data);
  }
}

auto POKEY::writeSKCTL(n8 data) -> void {
  control = data;
  serial.configure();
  if(control & 3) return;
  irq.writeEnable(0);
  status.resetErrors();
  keyboard.disable();
  serial.reset();
  clock.reset();
  setIRQ();
}

auto POKEY::startPots() -> void {
  n8 targets[8];
  for(u32 index = 0; index < 8; index++) {
    auto& port = controllerPorts[index >> 1];
    auto powered = gtia.controllerPower();
    if(!port.connected()) {
      targets[index] = powered ? 0xff : Pots::Maximum;
      continue;
    }
    auto value = (s32)port.axis(index & 1, powered);
    auto normalized = (u32)(value + 32768);
    auto maximum = powered ? Pots::Maximum - 1 : Pots::Maximum;
    targets[index] = (normalized * maximum + 32767u) / 65535u;
  }
  pots.start(targets);
}

auto POKEY::setIRQ() -> void {
  cpu.irqLine(irq.line());
}
