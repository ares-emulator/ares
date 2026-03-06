auto RIOT::readRam(n8 address) -> n8 {
  return ram[address];
}

auto RIOT::writeRam(n8 address, n8 data) -> void {
  ram[address] = data;
}

auto RIOT::readIo(n8 address) -> n8 {
  address &= 0x1f;

  switch(address) {
  case 0x00: return readPortA();
  case 0x01: return port[0].direction;
  case 0x02: return readPortB();
  case 0x03: return port[1].direction;
  case 0x04: case 0x06: {
    n8 data = timer.counter;

    if(timer.interruptFlag && !timer.justWrapped) {
      timer.interruptFlag = 0;
    }

    return data;
  }
  case 0x05: case 0x07: {
    n8 output;
    output.bit(6) = 0; //TODO: PA7 IRQ Flag
    output.bit(7) = timer.interruptFlag;
    return output;
  }
  }

  debug(unimplemented, "[RIOT] IO read ", hex(address));
  return 0xff;
}

auto RIOT::writeIo(n8 address, n8 data) -> void {
  address &= 0x1f;

  switch(address) {
  case 0x00: writePortA(data);         return;
  case 0x01: port[0].direction = data; return;
  case 0x02: writePortB(data);         return;
  case 0x03: port[1].direction = data; return;
  case 0x14: reloadTimer(data,    1, 0); return;
  case 0x15: reloadTimer(data,    8, 0); return;
  case 0x16: reloadTimer(data,   64, 0); return;
  case 0x17: reloadTimer(data, 1024, 0); return;
  case 0x1c: reloadTimer(data,    1, 1); return;
  case 0x1d: reloadTimer(data,    8, 1); return;
  case 0x1e: reloadTimer(data,   64, 1); return;
  case 0x1f: reloadTimer(data, 1024, 1); return;
  }

  debug(unimplemented, "[RIOT] IO write ", hex(address), " = ", hex(data));
}

auto RIOT::readPortA() -> n8 {
  n8 output = 0xff;

  n8 input;
  input.bit(0, 3) = controllerPort2.read().bit(0, 4);
  input.bit(4, 7) = controllerPort1.read().bit(0, 4);

  for(auto n : range(8)) {
    output.bit(n) = port[0].direction.bit(n)? port[0].data.bit(n) : input.bit(n);
  }

  return output;
}

auto RIOT::writePortA(n8 data) -> void {
  port[0].data = data;
}

auto RIOT::readPortB() -> n8 {
  system.controls.poll();

  n8 data = 0xff;

  for(auto n : range(8)) {
    if(port[1].direction.bit(n)) data.bit(n) = port[1].data.bit(n);
  }

  // Atari hard-wires these bits as input
  data.bit(0) = !system.controls.reset->value();
  data.bit(1) = !system.controls.select->value();

  // These are toggle switches; flip the values on the rising edge
  if(system.controls.tvType->value() && !tvTypeLatch) {
    tvType = !tvType;
  }
  tvTypeLatch = system.controls.tvType->value();

  if(system.controls.leftDifficulty->value() && !leftDifficultyLatch) {
    leftDifficulty = !leftDifficulty;
  }
  leftDifficultyLatch = system.controls.leftDifficulty->value();

  if(system.controls.rightDifficulty->value() && !rightDifficultyLatch) {
    rightDifficulty = !rightDifficulty;
  }
  rightDifficultyLatch = system.controls.rightDifficulty->value();

  data.bit(3) = tvType;
  data.bit(6) = leftDifficulty;
  data.bit(7) = rightDifficulty;

  return data;
}

auto RIOT::writePortB(n8 data) -> void {
  port[1].data = data;
}