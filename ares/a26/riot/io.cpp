auto RIOT::readRam(n8 address) -> n8 {
  return ram[address];
}

auto RIOT::writeRam(n8 address, n8 data) -> void {
  ram[address] = data;
}

auto RIOT::readIo(n8 address) -> n8 {
  switch(address) {
  case 0x00: return readPortA();
  case 0x01: return port[0].direction;
  case 0x02: return readPortB();
  case 0x03: return port[1].direction;
  case 0x04: return timer.counter;
  }

  debug(unimplemented, "[RIOT] IO read ", hex(address));
  return 0xff;
}

auto RIOT::writeIo(n8 address, n8 data) -> void {
  switch(address) {
  case 0x00: writePortA(data);         return;
  case 0x01: port[0].direction = data; return;
  case 0x02: writePortB(data);         return;
  case 0x03: port[1].direction = data; return;
  case 0x14: case 0x15: case 0x16: case 0x17: {
    const n16 intervals[] = {1, 8, 64, 1024};
    timer.counter = data;
    timer.interval = intervals[address - 0x14];
    timer.reload = timer.interval;
    return; }
  }

  debug(unimplemented, "[RIOT] IO write ",hex(address), " = ", hex(data));
}

auto RIOT::readPortA() -> n8 {
  n8 data;
  // TODO: Handle when configured as outputs
  data.bit(0, 3) = controllerPort2.read().bit(0, 4);
  data.bit(4, 7) = controllerPort1.read().bit(0, 4);
  return data;
}

auto RIOT::writePortA(n8 data) -> void {
  port[0].data = data;
}

auto RIOT::readPortB() -> n8 {
    system.controls.poll();

    n8 data;

    for(auto n : range(8)) if(port[1].direction.bit(n)) { data.bit(n) = port[1].data.bit(n); }

    // Atari hard-wires these bits as input
    data.bit(0) = !system.controls.reset->value();
    data.bit(1) = !system.controls.select->value();

    // These are toggle switches; flip the values on the rising edge
    if(system.controls.tvType->value() && !tvTypeLatch) {
      tvType = !tvType;
    }
    tvTypeLatch = system.controls.tvType->value();

    if(system.controls.leftDifficulty->value()  && !leftDifficultyLatch) {
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